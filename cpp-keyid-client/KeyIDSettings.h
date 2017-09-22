#pragma once
#include <string>

using namespace std;

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
	bool strictSSL;
};