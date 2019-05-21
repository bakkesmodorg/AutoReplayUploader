#pragma once

#include "HttpClient.h"

#include "bakkesmod/plugin/bakkesmodplugin.h"

#include <iostream>

using namespace std;

class Ballchasing
{
private:
	string UserAgent;
	string uploadBoundary;
	shared_ptr<CVarManagerWrapper> cvarManager = NULL;

public:
	Ballchasing(string userAgent, string uploadBoundary, shared_ptr<CVarManagerWrapper> cvarManager);
	~Ballchasing();

	void UploadReplay(string replayPath, string authKey, string visibility);
	void UploadCompleted(HttpRequestObject* ctx);
	bool TestAuthKey(string authKey);
};

