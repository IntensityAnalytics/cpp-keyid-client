#pragma once
#include <string>

struct KeyIDSettings
{
	std::wstring license = L"";
	std::wstring url = L"http://invalid.invalid";
	bool passiveValidation = false;
	bool passiveEnrollment = false;
	bool customThreshold = false;
	double thresholdConfidence = 70.0;
	double thresholdFidelity = 50.0;
	int timeout = 0;
	bool strictSSL = true;
};