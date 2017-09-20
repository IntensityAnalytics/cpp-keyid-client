#pragma once
#include "KeyIDService.h"
#include "KeyIDSettings.h"
#include <string>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/json.h>
#include <chrono>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

/// <summary>
/// KeyID client.
/// </summary>
class KeyIDClient
{
public:
	KeyIDClient(KeyIDSettings settings);
	~KeyIDClient();
	pplx::task<json::value> saveProfile(wstring entityID, wstring tsData, wstring sessionID = L"");
	pplx::task<json::value> removeProfile(wstring entityID, wstring tsData = L"", wstring sessionID = L"");
	pplx::task<json::value> evaluateProfile(wstring entityID, wstring tsData, wstring sessionID = L"");
	pplx::task<json::value> loginPassiveEnrollment(wstring entityID, wstring tsData, wstring sessionID = L"");

private:
	KeyIDService *service;
	KeyIDSettings settings;
	bool evalThreshold(double confidence, double fidelity);
	bool alphaToBool(wstring input);
	long long dotNetTicks();
	json::value parseResponse(const http_response& response);
};