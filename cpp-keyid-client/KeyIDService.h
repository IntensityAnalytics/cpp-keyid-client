#pragma once
#include "afxwin.h"
#include <string>
#include <cpprest/http_client.h>
#include <cpprest/json.h>

/// <summary>
///  KeyID services REST client.
/// </summary>
class KeyIDService
{
public:
	KeyIDService(std::wstring url, std::wstring license, int timeoutMs = 1000);
	~KeyIDService();
	pplx::task<web::http::http_response> TypingMistake(std::wstring entityID, std::wstring mistype = L"", std::wstring sessionID = L"", std::wstring source = L"", std::wstring action = L"", std::wstring tmplate = L"", std::wstring page = L"");
	pplx::task<web::http::http_response> EvaluateSample(std::wstring entityID, std::wstring tsData, std::wstring nonce);
	pplx::task<web::http::http_response> Nonce(long long nonceTime);
	pplx::task<web::http::http_response> RemoveToken(std::wstring entityID, std::wstring tsData);
	pplx::task<web::http::http_response> RemoveProfile(std::wstring entityID, std::wstring token);
	pplx::task<web::http::http_response> SaveToken(std::wstring entityID, std::wstring tsData);
	pplx::task<web::http::http_response> SaveProfile(std::wstring entityID, std::wstring tsData, std::wstring code = L"");
	pplx::task<web::http::http_response> GetProfileInfo(std::wstring entityID);
	pplx::task<web::http::http_response> ErrorLog(std::wstring entityID, std::wstring message, std::wstring source, std::wstring machine);
	pplx::task<web::http::http_response> Mistype(std::wstring entityID);

private:
	std::wstring url;
	std::wstring license;
	std::shared_ptr<web::http::client::http_client> client;

	web::json::value encodeJSONProperties(web::json::value obj);
	pplx::task<web::http::http_response> Post(std::wstring path, web::json::value data);
	pplx::task<web::http::http_response> Get(std::wstring path, web::json::value data);
};