#pragma once
#include "KeyIDService.h"
#include "KeyIDSettings.h"
#include <string>
#include <cpprest/http_client.h>
#include <cpprest/json.h>

/// <summary>
/// KeyID client.
/// </summary>
class KeyIDClient
{
public:
	KeyIDClient(KeyIDSettings settings);
	KeyIDClient();
	~KeyIDClient();
	const KeyIDSettings& GetSettings();
	void SetSettings(KeyIDSettings settings);

	pplx::task<web::json::value> SaveProfile(std::wstring entityID, std::wstring tsData, std::wstring sessionID = L"");
	pplx::task<web::json::value> RemoveProfile(std::wstring entityID, std::wstring tsData = L"", std::wstring sessionID = L"");
	pplx::task<web::json::value> EvaluateProfile(std::wstring entityID, std::wstring tsData, std::wstring sessionID = L"");
	pplx::task<web::json::value> LoginPassiveEnrollment(std::wstring entityID, std::wstring tsData, std::wstring sessionID = L"");
	pplx::task<web::json::value> GetProfileInfo(std::wstring entityID);

private:
	std::shared_ptr<KeyIDService> service;
	KeyIDSettings settings;

	bool EvalThreshold(double confidence, double fidelity);
	bool AlphaToBool(std::wstring input);
	long long DotNetTicks();
	web::json::value ParseResponse(const web::http::http_response& response);
	web::json::value ParseGetProfileResponse(const web::http::http_response & response);
};