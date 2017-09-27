# cpp-keyid-client

A cpp library to allow you to peform second factor authentication using Intensity Analytics TickStream.KeyID.

## Install

Currently provided as a static library - soon to be deployed as a NuGet package / DLL with CLR compatibility.

## Usage

The keyid-client library provides several asynchronous functions that return Casablanca PPLX tasks.

```cpp
#include "..\cpp-keyid-client\KeyIDClient.h"

using namespace std;
using namespace web;

void main(){
	KeyIDSettings settings;
	settings.license = L"yourlicensekey";
	settings.url = L"https://keyidservicesurl";
	settings.passiveEnrollment = false;
	settings.passiveValidation = false;
	settings.customThreshold = false;
	settings.thresholdConfidence = 70.0;
	settings.thresholdFidelity = 50.0;
	settings.timeout = 1000;

	KeyIDClient client = KeyIDClient(settings);

	wstring entityID = L"someusername";
	wstring tsData = L"tsdata captured from a textbox using keyid browser javscript library";

	client.RemoveProfile(entityID)
	.wait();

	client.SaveProfile(entityID, tsData)
	.wait();

	client.EvaluateProfile(entityID, tsData)
	.then([](json::value data) {
		wstring Match = data.at(L"Match").as_string();
		wstring Confidence = data.at(L"Confidence").as_string();
		wstring Fidelity = data.at(L"Fidelity").as_string();
	})
	.wait();
}
```