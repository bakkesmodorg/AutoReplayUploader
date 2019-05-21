#pragma once

#include "HttpClient.h"

#include "bakkesmod/plugin/bakkesmodplugin.h"

#include <iostream>

using namespace std;

class Calculated
{
private:
	string UserAgent;
	string UploadBoundary;
	shared_ptr<CVarManagerWrapper> cvarManager = NULL;

public:
	Calculated(string userAgent, string uploadBoundary, shared_ptr<CVarManagerWrapper> cvarManager);
	~Calculated();

	void UploadReplay(string replayPath);
	void UploadCompleted(HttpRequestObject* ctx);
};

