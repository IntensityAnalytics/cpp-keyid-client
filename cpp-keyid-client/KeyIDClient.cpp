#include "KeyIDClient.h"
#include <cpprest/filestream.h>
#include <chrono>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

/// <summary>
/// KeyID services client.
/// </summary>
/// <param name="settings"> KeyID settings struct</param>
KeyIDClient::KeyIDClient(KeyIDSettings settings)
{
	this->settings = settings;
	this->service = make_shared<KeyIDService>(settings.url, settings.license, settings.timeout);
}

KeyIDClient::KeyIDClient()
{
	this->service = make_shared<KeyIDService>(settings.url, settings.license, settings.timeout);
}

/// <summary>
/// KeyID client destructor.
/// </summary>
KeyIDClient::~KeyIDClient()
{
}

const KeyIDSettings& KeyIDClient::GetSettings()
{
	return settings;
}

void KeyIDClient::SetSettings(KeyIDSettings settings)
{
	this->settings = settings;
}

/// <summary>
/// Saves a given KeyID profile entry.
/// </summary>
/// <param name="entityID">Profile name to save.</param>
/// <param name="tsData">Typing sample data to save.</param>
/// <param name="sessionID">Session identifier for logging purposes.</param>
/// <returns>JSON value (task)</returns>
pplx::task<web::json::value> KeyIDClient::SaveProfile(std::wstring entityID, std::wstring tsData, std::wstring sessionID)
{
	// try to save profile without a token
	return service->SaveProfile(entityID, tsData)
	.then([=](http_response response)
	{
		json::value data = ParseResponse(response);

		if (data[L"Error"].as_string() == L"Invalid license key.")
			throw exception("Invalid license key.");

		// token is required
		if (data[L"Error"].as_string() == L"New enrollment code required.")
		{
			// get a save token
			return service->SaveToken(entityID, tsData)
			.then([=](http_response response)
			{
				json::value data = ParseResponse(response);
				// try to save profile with a token
				return service->SaveProfile(entityID, tsData, data[L"Token"].as_string());
			})
			.then([=](http_response response)
			{
				json::value data = ParseResponse(response);
				//todo this isn't a task?
				return data;
			});
		}

		return pplx::task_from_result(data);
	});
}

/// <summary>
/// Removes a KeyID profile.
/// </summary>
/// <param name="entityID">Profile name to remove.</param>
/// <param name="tsData">Optional typing sample for removal authorization.</param>
/// <param name="sessionID">Session identifier for logging purposes.</param>
/// <returns>JSON value (task)</returns>
pplx::task<web::json::value> KeyIDClient::RemoveProfile(std::wstring entityID, std::wstring tsData, std::wstring sessionID)
{
	// get a removal token
	return service->RemoveToken(entityID, tsData)
	.then([=](http_response response)
	{
		json::value data = ParseResponse(response);

		if (data[L"Error"].as_string() == L"Invalid license key.")
			throw exception("Invalid license key.");

		// remove profile
		if (data.has_field(L"Token")) {
			return service->RemoveProfile(entityID, data[L"Token"].as_string())
				.then([=](http_response response)
			{
				json::value data = ParseResponse(response);
				return pplx::task_from_result(data);
			});
		}
		else
			return pplx::task_from_result(data);
	});
}

/// <summary>
/// Evaluates a KeyID profile.
/// </summary>
/// <param name="entityID">Profile name to evaluate.</param>
/// <param name="tsData">Typing sample to evaluate against profile.</param>
/// <param name="sessionID">Session identifier for logging purposes.</param>
/// <returns></returns>
pplx::task<web::json::value> KeyIDClient::EvaluateProfile(std::wstring entityID, std::wstring tsData, std::wstring sessionID)
{
	long long nonceTime = DotNetTicks();

	return service->Nonce(nonceTime)
	.then([=](http_response response)
	{
		return service->EvaluateSample(entityID, tsData, response.extract_string().get());
	})
	.then([=](http_response response)
	{
		json::value data = ParseResponse(response);

		if (data[L"Error"].as_string() == L"Invalid license key.")
			throw exception("Invalid license key.");

		// check for error before continuing
		// todo check "error" exists first?
		if (data[L"Error"].as_string() == L"")
		{
			// coerce string to boolean
			data[L"Match"] = json::value::boolean(AlphaToBool(data[L"Match"].as_string()));
			data[L"IsReady"] = json::value::boolean(AlphaToBool(data[L"IsReady"].as_string()));

			// set match to true and return early if using passive validation
			if (settings.passiveValidation)
			{
				data[L"Match"] = json::value::boolean(true);
				return data;
			}
			// evaluate match value using custom threshold if enabled
			else if (settings.customThreshold)
			{
				data[L"Match"] = json::value::boolean(EvalThreshold(data[L"Confidence"].as_double(), data[L"Fidelity"].as_double()));
			}
		}

		return data;
	});
}

/// <summary>
/// Evaluates a given profile and adds typing sample to profile.
/// </summary>
/// <param name="entityID">Profile to evaluate.</param>
/// <param name="tsData">Typing sample to evaluate and save.</param>
/// <param name="sessionID">Session identifier for logging purposes.</param>
/// <returns></returns>
pplx::task<web::json::value> KeyIDClient::LoginPassiveEnrollment(std::wstring entityID, std::wstring tsData, std::wstring sessionID)
{
	return EvaluateProfile(entityID, tsData, sessionID)
	.then([=](json::value data)
	{	
		// in base case that no profile exists save profile async and return early
		if (data[L"Error"].as_string() == L"EntityID does not exist." ||
			data[L"Error"].as_string() == L"The profile has too little data for a valid evaluation." ||
			data[L"Error"].as_string() == L"The entry varied so much from the model, no evaluation is possible.")
		{
			return SaveProfile(entityID, tsData, sessionID)
			.then([=](json::value saveData)
			{
				json::value evalData = data;
				evalData[L"Match"] = json::value::boolean(true);
				evalData[L"IsReady"] = json::value::boolean(false);
				evalData[L"Confidence"] = json::value::number(100.0);
				evalData[L"Fidelity"] = json::value::number(100.0);
				return evalData;
			});
		}

		// if profile is not ready save profile async and return early
		if (data[L"Error"].as_string() == L"" && data[L"IsReady"].as_bool() == false)
		{
			return SaveProfile(entityID, tsData, sessionID)
			.then([=](json::value saveData)
			{
				json::value evalData = data;
				evalData[L"Match"] = json::value::boolean(true);
				return evalData;
			});
		}

		return pplx::task_from_result(data);
	});
}

/// <summary>
/// Returns profile information without modifying the profile.
/// </summary>
/// <param name="entityID">Profile to inspect.</param>
/// <returns></returns>
pplx::task<web::json::value> KeyIDClient::GetProfileInfo(std::wstring entityID)
{
	return service->GetProfileInfo(entityID)
	.then([=](http_response response)
	{
		json::value data = ParseGetProfileResponse(response);
		return pplx::task_from_result(data);
	});
}

/// <summary>
/// Write to server error log.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="message">Error message.</param>
/// <param name="source">Source (application, DLL, etc.)</param>
/// <param name="machine">Machine (IP address, machine GUID, etc.)</param>
pplx::task<web::json::value> KeyIDClient::ErrorLog(std::wstring entityID, std::wstring message, std::wstring source, std::wstring machine)
{
	return service->ErrorLog(entityID, message, source, machine)
		.then([=](http_response response)
	{
		json::value data = ParseResponse(response);
		return pplx::task_from_result(data);
	});
}

/// <summary>
/// Notify server of a mistype.
/// </summary>
/// <param name="entityID">Profile name.</param>
pplx::task<web::json::value> KeyIDClient::Mistype(std::wstring entityID)
{
	return service->Mistype(entityID)
		.then([=](http_response response)
	{
		json::value data = ParseResponse(response);
		return pplx::task_from_result(data);
	});
}

/// <summary>
/// Compares a given confidence and fidelity against pre-determined thresholds.
/// </summary>
/// <param name="confidence">KeyID evaluation confidence.</param>
/// <param name="fidelity">KeyID evaluation fidelity.</param>
/// <returns>Whether confidence and fidelity meet thresholds.</returns>
bool KeyIDClient::EvalThreshold(double confidence, double fidelity)
{
	if (confidence >= settings.thresholdConfidence &&
		fidelity >= settings.thresholdFidelity)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/// <summary>
/// Converts a string value like 'true' to a boolean object.
/// </summary>
/// <param name="input">String to convert to boolean.</param>
/// <returns>Boolean value.</returns>
bool KeyIDClient::AlphaToBool(std::wstring input)
{
	std::transform(input.begin(), input.end(), input.begin(), ::toupper);

	if (input == L"TRUE")
		return true;
	else
		return false;
}

/// <summary>
/// Converts current time to Microsoft .Net 'ticks'. A tick is 100 nanoseconds.
/// </summary>
/// <returns>Current time in 'ticks'.</returns>
long long KeyIDClient::DotNetTicks()
{
	const long long EPOCH_OFFSET = 621355968000000000;
	const long MS_PER_TICK = 10000;
	long long ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	long long ticks = ((ms_since_epoch * MS_PER_TICK) + EPOCH_OFFSET);
	return ticks;
}

/// <summary>
/// Extracts a JSON value from a http_response
/// </summary>
/// <param name="response">HTTP response</param>
/// <returns>JSON value</returns>
web::json::value KeyIDClient::ParseResponse(const web::http::http_response& response)
{
	if (response.status_code() == status_codes::OK)
	{
		return response.extract_json().get();
	}
	else
	{
		throw http_exception(L"HTTP response not 200 OK.");
	}
}

/// <summary>
/// Extracts a JSON value from a http_response
/// </summary>
/// <param name="response">HTTP response</param>
/// <returns>JSON value</returns>
web::json::value KeyIDClient::ParseGetProfileResponse(const web::http::http_response& response)
{
	if (response.status_code() == status_codes::OK)
	{
		json::value data = response.extract_json().get();
		if (data.is_array())
			return data[0];
		else
			return data;
	}
	else
	{
		throw http_exception(L"HTTP response not 200 OK.");
	}
}