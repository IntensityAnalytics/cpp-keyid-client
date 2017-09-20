#include "KeyIDClient.h"

/// <summary>
/// KeyID services client.
/// </summary>
/// <param name="settings"> KeyID settings struct</param>
KeyIDClient::KeyIDClient(KeyIDSettings settings)
{
	this->settings = settings;
	this->service = new KeyIDService(settings.url, settings.license, settings.timeout);
}

/// <summary>
/// KeyID client destructor.
/// </summary>
KeyIDClient::~KeyIDClient()
{
	delete service;
}

/// <summary>
/// Saves a given KeyID profile entry.
/// </summary>
/// <param name="entityID">Profile name to save.</param>
/// <param name="tsData">Typing sample data to save.</param>
/// <param name="sessionID">Session identifier for logging purposes.</param>
/// <returns>JSON value (task)</returns>
pplx::task<json::value> KeyIDClient::saveProfile(wstring entityID, wstring tsData, wstring sessionID)
{
	// try to save profile without a token
	return service->saveProfile(entityID, tsData)
	.then([=](http_response response)
	{
		json::value data = parseResponse(response);

		// token is required
		if (wcscmp(data[L"Error"].as_string().c_str(), L"") != 0)
		{
			// get a save token
			return service->saveToken(entityID, tsData)
			.then([=](http_response response)
			{
				json::value data = parseResponse(response);
				// try to save profile with a token
				return service->saveProfile(entityID, tsData, data[L"Token"].as_string());
			})
			.then([=](http_response response)
			{
				json::value data = parseResponse(response);
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
pplx::task<json::value> KeyIDClient::removeProfile(wstring entityID, wstring tsData, wstring sessionID)
{
	// get a removal token
	return service->removeToken(entityID, tsData).then([=](http_response response)
	{
		json::value data = parseResponse(response);

		// remove profile
		return service->removeProfile(entityID, data[L"Token"].as_string())
		.then([=](http_response response)
		{
			json::value data = parseResponse(response);
			return pplx::task_from_result(data);
		});
	});
}

/// <summary>
/// Evaluates a KeyID profile.
/// </summary>
/// <param name="entityID">Profile name to evaluate.</param>
/// <param name="tsData">Typing sample to evaluate against profile.</param>
/// <param name="sessionID">Session identifier for logging purposes.</param>
/// <returns></returns>
pplx::task<json::value> KeyIDClient::evaluateProfile(wstring entityID, wstring tsData, wstring sessionID)
{
	long long nonceTime = dotNetTicks();

	return service->nonce(nonceTime)
	.then([=](http_response response)
	{
		return service->evaluateSample(entityID, tsData, response.extract_string().get());
	})
	.then([=](http_response response)
	{
		json::value data = parseResponse(response);

		// return early if profile does not exist
		if (wcscmp(data[L"Error"].as_string().c_str(), L"EntityID does not exist.") == 0)
		{
			return data;
		}

		// coerce string to boolean
		data[L"Match"] = json::value::boolean(alphaToBool(data[L"Match"].as_string()));
		data[L"IsReady"] = json::value::boolean(alphaToBool(data[L"IsReady"].as_string()));

		// set match to true and return early if using passive validation
		if (settings.passiveValidation)
		{
			data[L"Match"] = json::value::boolean(true);
			return data;
		}
		// evaluate match value using custom threshold if enabled
		else if (settings.customThreshold)
		{
			data[L"Match"] = json::value::boolean(evalThreshold(data[L"Confidence"].as_double(), data[L"Fidelity"].as_double()));
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
pplx::task<json::value> KeyIDClient::loginPassiveEnrollment(wstring entityID, wstring tsData, wstring sessionID)
{
	return evaluateProfile(entityID, tsData, sessionID)
	.then([=](json::value data)
	{
		// in base case that no profile exists save profile async and return early
		if (wcscmp(data[L"Error"].as_string().c_str(), L"EntityID does not exist.") == 0 ||
			wcscmp(data[L"Error"].as_string().c_str(), L"The profile has too little data for a valid evaluation.") == 0 ||
			wcscmp(data[L"Error"].as_string().c_str(), L"The entry varied so much from the model, no evaluation is possible.") == 0)
		{
			return saveProfile(entityID, tsData, sessionID)
			.then([=](json::value saveData)
			{
				json::value evalData = data;
				evalData[L"Match"] = json::value::boolean(true);
				evalData[L"Confidence"] = json::value::number(100.0);
				evalData[L"Fidelity"] = json::value::number(100.0);
				return evalData;
			});
		}

		// if profile is not ready save profile async and return early
		if (data[L"IsReady"].as_bool() == false)
		{
			return saveProfile(entityID, tsData, sessionID)
			.then([=](json::value saveData)
			{
				json::value evalData = data;
				evalData[L"Match"] = json::value::boolean(true);
				return evalData;
			});
		}

		//return pplx::create_task([data]()->json::value {return data; });
		return pplx::task_from_result(data);
	});
}

/// <summary>
/// Compares a given confidence and fidelity against pre-determined thresholds.
/// </summary>
/// <param name="confidence">KeyID evaluation confidence.</param>
/// <param name="fidelity">KeyID evaluation fidelity.</param>
/// <returns>Whether confidence and fidelity meet thresholds.</returns>
bool KeyIDClient::evalThreshold(double confidence, double fidelity)
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
bool KeyIDClient::alphaToBool(wstring input)
{
	std::transform(input.begin(), input.end(), input.begin(), ::toupper);

	if (wcscmp(input.c_str(), L"TRUE") == 0)
		return true;
	else
		return false;
}

/// <summary>
/// Converts current time to Microsoft .Net 'ticks'. A tick is 100 nanoseconds.
/// </summary>
/// <returns>Current time in 'ticks'.</returns>
long long KeyIDClient::dotNetTicks()
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
json::value KeyIDClient::parseResponse(const http_response& response)
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