#include "KeyIDService.h"

/// <summary>
/// KeyID services REST client.
/// </summary>
/// <param name="url">KeyID services URL.</param>
/// <param name="license">KeyID services license key.</param>
/// <param name="timeoutMs">REST web service timeout.</param>
KeyIDService::KeyIDService(wstring url, wstring license, int timeoutMs)
{
	this->url = url;
	this->license = license;
	client = new http_client(url);
}

/// <summary>
/// Keyid services destructor.
/// </summary>
KeyIDService::~KeyIDService()
{
	delete client;
}

/// <summary>
/// URL encodes the properties of a JSON object
/// </summary>
/// <param name="obj">JSON object</param>
/// <returns>URL encoded JSON object</returns>
json::value KeyIDService::encodeJSONProperties(json::value obj)
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
pplx::task<http_response> KeyIDService::post(wstring path, json::value data)
{
	data[L"License"] = json::value::string(license);
	json::value dataEncoded = encodeJSONProperties(data);
	wstring dataEncodedJSON = dataEncoded.serialize();

	return client->request(methods::POST,
						   path,
						   L"=[" + dataEncodedJSON + L"]", 
						   L"application/x-www-form-urlencoded");
}

/// <summary>
/// Performs a HTTP get to KeyID REST services.
/// </summary>
/// <param name="path">REST URI suffix.</param>
/// <param name="data">Object that will be converted to URL parameters and sent in GET request.</param>
/// <returns>REST request and response.</returns>
pplx::task<http_response> KeyIDService::get(wstring path, json::value data)
{
	uri_builder params(path);

	// iterate json data and add to query parameters
	for (auto &jsonTuple : data.as_object())
	{
		params.append_query(jsonTuple.first, jsonTuple.second.as_string());
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
pplx::task<http_response> KeyIDService::typingMistake(wstring entityID, wstring mistype, wstring sessionID, wstring source, wstring action, wstring tmplate, wstring page )
{
	json::value data;
	data[L"EntityID"] = json::value::string(entityID);
	data[L"Mistype"] = json::value::string(mistype);
	data[L"SessionID"] = json::value::string(sessionID);
	data[L"Source"] = json::value::string(source);
	data[L"Action"] = json::value::string(action);
	data[L"Template"] = json::value::string(tmplate);
	data[L"Page"] = json::value::string(page);

	return post(L"/typingmistake", data);
}

/// <summary>
/// Evaluate typing sample.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="tsData">Typing sample to evaluate against profile.</param>
/// <param name="nonce">Evaluation nonce.</param>
/// <returns>REST request and response.</returns>
pplx::task<http_response> KeyIDService::evaluateSample(wstring entityID, wstring tsData, wstring nonce)
{
	json::value data;
	data[L"EntityID"] = json::value::string(entityID);
	data[L"tsData"] = json::value::string(tsData);
	data[L"Nonce"] = json::value::string(nonce);
	data[L"Return"] = json::value::string(L"JSON");
	data[L"Statistics"] = json::value::string(L"extended");

	return post(L"/evaluate", data);
}

/// <summary>
/// Retrieve a nonce.
/// </summary>
/// <param name="nonceTime">Current time in .Net ticks.</param>
/// <returns>REST request and response.</returns>
pplx::task<http_response> KeyIDService::nonce(long long nonceTime)
{
	json::value data;
	data[L"type"] = json::value::string(L"nonce");
	wstring path = L"/token/" + to_wstring(nonceTime);
	return get(path, data);
}

/// <summary>
/// Retrieve a profile removal security token.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="tsData">Optional typing sample for removal authorization.</param>
/// <returns>REST request and response.</returns>
pplx::task<http_response> KeyIDService::removeToken(wstring entityID, wstring tsData)
{
	json::value data;
	data[L"Type"] = json::value::string(L"remove");
	data[L"Return"] = json::value::string(L"value");

	return get(L"/token/" + entityID, data)
	.then([=](http_response response)
	{
		json::value postData;
		postData[L"EntityID"] = json::value::string(entityID);
		postData[L"Token"] = json::value::string(response.extract_string().get().c_str());
		postData[L"ReturnToken"] = json::value::string(L"True");
		postData[L"ReturnValidation"] = json::value::string(tsData);
		postData[L"Type"] = json::value::string(L"remove");
		postData[L"Return"] = json::value::string(L"JSON");

		return post(L"/token", postData);
	});
}

/// <summary>
/// Remove a profile.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="token">Profile removal security token.</param>
/// <returns>REST request and response.</returns>
pplx::task<http_response> KeyIDService::removeProfile(wstring entityID, wstring token)
{
	json::value data;
	data[L"EntityID"] = json::value::string(entityID);
	data[L"Code"] = json::value::string(token);
	data[L"Action"] = json::value::string(L"remove");
	data[L"Return"] = json::value::string(L"JSON");

	return post(L"/profile", data);
}

/// <summary>
/// Retrieve a profile save security token.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="tsData">Optional typing sample for save authorization.</param>
/// <returns>REST request and response.</returns>
pplx::task<http_response> KeyIDService::saveToken(wstring entityID, wstring tsData)
{
	json::value data;
	data[L"Type"] = json::value::string(L"remove");
	data[L"Return"] = json::value::string(L"value");

	return get(L"/token/" + entityID, data)
	.then([=](http_response response)
	{
		json::value postData;
		postData[L"EntityID"] = json::value::string(entityID);
		postData[L"Token"] = json::value::string(response.extract_string().get().c_str());
		postData[L"ReturnToken"] = json::value::string(L"True");
		postData[L"ReturnValidation"] = json::value::string(tsData);
		postData[L"Type"] = json::value::string(L"enrollment");
		postData[L"Return"] = json::value::string(L"JSON");

		return post(L"/token", postData);
	});
}

/// <summary>
/// Save a profile.
/// </summary>
/// <param name="entityID">Profile name.</param>
/// <param name="tsData">Typing sample to save.</param>
/// <param name="code">Profile save security token.</param>
/// <returns>REST request and response.</returns>
pplx::task<http_response> KeyIDService::saveProfile(wstring entityID, wstring tsData, wstring code)
{
	json::value data;
	data[L"EntityID"] = json::value::string(entityID);
	data[L"tsData"] = json::value::string(tsData);
	data[L"Return"] = json::value::string(L"JSON");
	data[L"Action"] = json::value::string(L"v2");
	data[L"Statistics"] = json::value::string(L"extended");

	if (wcscmp(code.c_str(), L"") == 0)
		data[L"Code"] = json::value::string(code);

	return post(L"/profile", data);
}
