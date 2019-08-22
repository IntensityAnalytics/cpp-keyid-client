#include "KeyIDService.h"
#include <cpprest/filestream.h>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

/// <summary>
/// KeyID services REST client.
/// </summary>
/// <param name="url">KeyID services URL.</param>
/// <param name="license">KeyID services license key.</param>
/// <param name="timeoutMs">REST web service timeout.</param>
KeyIDService::KeyIDService(std::wstring url, std::wstring license, int timeoutMs)
{
	this->url = url;
	this->license = license;
	client = std::make_shared<web::http::client::http_client>(url);
}

/// <summary>
/// Keyid services destructor.
/// </summary>
KeyIDService::~KeyIDService()
{
}

/// <summary>
/// URL encodes the properties of a JSON object
/// </summary>
/// <param name="obj">JSON object</param>
/// <returns>URL encoded JSON object</returns>
web::json::value KeyIDService::encodeJSONProperties(web::json::value obj)
{
	json::value objEncoded;

	for (auto &jsonTuple : obj.as_object())
	{
		wstring encodedData = uri::encode_data_string(jsonTuple.second.as_string());
		objEncoded[jsonTuple.first] = json::value::string(encodedData);
	}

	return objEncoded;
}

/// <summary>
/// Performs a HTTP post to KeyID REST services.
/// </summary>
/// <param name="path">REST URI suffix.</param>
/// <param name="data">Object that will be converted to JSON and sent in POST request.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::Post(std::wstring path, web::json::value data)
{
	//data[L"License"] = json::value::string(license);
	json::value dataEncoded = encodeJSONProperties(data);
	wstring dataEncodedJSON = dataEncoded.serialize();
	
	http_request req(methods::POST);
	req.headers().set_content_type(L"application/x-www-form-urlencoded");
	req.headers().add(header_names::authorization, L"Bearer " + license);
	req.set_body(L"=[" + dataEncodedJSON + L"]", L"application/x-www-form-urlencoded");
	req.set_request_uri(path);
	return client->request(req);
}

/// <summary>
/// Performs a HTTP get to KeyID REST services.
/// </summary>
/// <param name="path">REST URI suffix.</param>
/// <param name="data">Object that will be converted to URL parameters and sent in GET request.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::Get(std::wstring path, web::json::value data)
{
	uri_builder params(path);

	// iterate json data and add to query parameters
	if (!data.is_null()) {
		for (auto &jsonTuple : data.as_object())
		{
			params.append_query(jsonTuple.first, jsonTuple.second.as_string());
		}
	}

	return client->request(methods::GET, params.to_string());
}

/// <summary>
/// Log a typing mistake to KeyID REST services.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="mistype">Typing mistake.</param>
/// <param name="sessionID">Session identifier for logging purposes.</param>
/// <param name="source">Application name or identifier.</param>
/// <param name="action">Action being performed at time of mistake.</param>
/// <param name="tmplate"></param>
/// <param name="page"></param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::TypingMistake(std::wstring entityID, std::wstring mistype, std::wstring sessionID, std::wstring source, std::wstring action, std::wstring tmplate, std::wstring page )
{
	json::value data;
	data[L"EntityID"] = json::value::string(entityID);
	data[L"Mistype"] = json::value::string(mistype);
	data[L"SessionID"] = json::value::string(sessionID);
	data[L"Source"] = json::value::string(source);
	data[L"Action"] = json::value::string(action);
	data[L"Template"] = json::value::string(tmplate);
	data[L"Page"] = json::value::string(page);

	return Post(L"/typingmistake", data);
}

/// <summary>
/// Evaluate typing sample.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="tsData">Typing sample to evaluate against profile.</param>
/// <param name="nonce">Evaluation nonce.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::EvaluateSample(std::wstring entityID, std::wstring tsData, std::wstring nonce)
{
	json::value data;
	data[L"EntityID"] = json::value::string(entityID);
	data[L"tsData"] = json::value::string(tsData);
	data[L"Nonce"] = json::value::string(nonce);
	data[L"Return"] = json::value::string(L"JSON");
	data[L"Statistics"] = json::value::string(L"extended");

	return Post(L"/evaluate", data);
}

/// <summary>
/// Retrieve a nonce.
/// </summary>
/// <param name="nonceTime">Current time in .Net ticks.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::Nonce(long long nonceTime)
{
	json::value data;
	data[L"type"] = json::value::string(L"nonce");
	wstring path = L"/token/" + to_wstring(nonceTime);
	return Get(path, data);
}

/// <summary>
/// Retrieve a profile removal security token.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="tsData">Optional typing sample for removal authorization.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::RemoveToken(std::wstring entityID, std::wstring tsData)
{
	json::value data;
	data[L"Type"] = json::value::string(L"remove");
	data[L"Return"] = json::value::string(L"value");

	return Get(L"/token/" + entityID, data)
	.then([=](http_response response)
	{
		json::value postData;
		postData[L"EntityID"] = json::value::string(entityID);
		postData[L"Token"] = json::value::string(response.extract_string().get().c_str());
		postData[L"ReturnToken"] = json::value::string(L"True");
		postData[L"ReturnValidation"] = json::value::string(tsData);
		postData[L"Type"] = json::value::string(L"remove");
		postData[L"Return"] = json::value::string(L"JSON");

		return Post(L"/token", postData);
	});
}

/// <summary>
/// Remove a profile.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="token">Profile removal security token.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::RemoveProfile(std::wstring entityID, std::wstring token)
{
	json::value data;
	data[L"EntityID"] = json::value::string(entityID);
	data[L"Code"] = json::value::string(token);
	data[L"Action"] = json::value::string(L"remove");
	data[L"Return"] = json::value::string(L"JSON");

	return Post(L"/profile", data);
}

/// <summary>
/// Retrieve a profile save security token.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="tsData">Optional typing sample for save authorization.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::SaveToken(std::wstring entityID, std::wstring tsData)
{
	json::value data;
	data[L"Type"] = json::value::string(L"enrollment");
	data[L"Return"] = json::value::string(L"value");

	return Get(L"/token/" + entityID, data)
	.then([=](http_response response)
	{
		json::value postData;
		postData[L"EntityID"] = json::value::string(entityID);
		postData[L"Token"] = json::value::string(response.extract_string().get().c_str());
		postData[L"ReturnToken"] = json::value::string(L"True");
		postData[L"ReturnValidation"] = json::value::string(tsData);
		postData[L"Type"] = json::value::string(L"enrollment");
		postData[L"Return"] = json::value::string(L"JSON");

		return Post(L"/token", postData);
	});
}

/// <summary>
/// Save a profile.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="tsData">Typing sample to save.</param>
/// <param name="code">Profile save security token.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::SaveProfile(std::wstring entityID, std::wstring tsData, std::wstring code)
{
	json::value data;
	data[L"EntityID"] = json::value::string(entityID);
	data[L"tsData"] = json::value::string(tsData);
	data[L"Return"] = json::value::string(L"JSON");
	data[L"Action"] = json::value::string(L"v2");
	data[L"Statistics"] = json::value::string(L"extended");

	if (code != L"")
		data[L"Code"] = json::value::string(code);

	return Post(L"/profile", data);
}

/// <summary>
/// Get profile information.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <returns>REST request and response.</returns>
pplx::task<web::http::http_response> KeyIDService::GetProfileInfo(std::wstring entityID)
{
	json::value data;
	wstring path = L"/profile/" + entityID;
	return Get(path, data);
}
