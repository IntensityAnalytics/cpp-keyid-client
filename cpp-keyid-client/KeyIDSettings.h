#pragma once
#include <string>

struct KeyIDSettings
{
	wstring license;
	wstring url;
	bool passiveValidation;
	bool passiveEnrollment;
	bool customThreshold;
	double thresholdConfidence;
	double thresholdFidelity;
	int timeout;
};