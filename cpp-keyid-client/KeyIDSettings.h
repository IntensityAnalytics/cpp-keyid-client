#pragma once
#include <string>

struct KeyIDSettings
{
	std::wstring license;
	std::wstring url;
	bool passiveValidation;
	bool passiveEnrollment;
	bool customThreshold;
	double thresholdConfidence;
	double thresholdFidelity;
	int timeout;
	bool strictSSL;
};