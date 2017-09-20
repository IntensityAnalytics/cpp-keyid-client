#pragma once
#include "afxwin.h"
#include <string>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/json.h>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

/// <summary>
///  KeyID services REST client.
/// </summary>
class KeyIDService
{
public:
	KeyIDService(wstring url, wstring license, int timeoutMs = 1000);
	~KeyIDService();
	pplx::task<http_response> typingMistake(wstring entityID, wstring mistype = L"", wstring sessionID = L"", wstring source = L"", wstring action = L"", wstring tmplate = L"", wstring page = L"");
	pplx::task<http_response> evaluateSample(wstring entityID, wstring tsData, wstring nonce);
	pplx::task<http_response> nonce(long long nonceTime);
	pplx::task<http_response> removeToken(wstring entityID, wstring tsData);
	pplx::task<http_response> removeProfile(wstring entityID, wstring token);
	pplx::task<http_response> saveToken(wstring entityID, wstring tsData);
	pplx::task<http_response> saveProfile(wstring entityID, wstring tsData, wstring code = L"");

private:
	wstring url;
	wstring license;
	http_client* client;

	json::value encodeJSONProperties(json::value obj);
	pplx::task<http_response> post(wstring path, json::value data);
	pplx::task<http_response> get(wstring path, json::value data);
};