#pragma once

#include "HttpClient.h"

#include "bakkesmod/plugin/bakkesmodplugin.h"

#include <iostream>

using namespace std;

#define CVAR_BALLCHASING_AUTH_KEY "cl_autoreplayupload_ballchasing_authkey"
#define CVAR_BALLCHASING_AUTH_TEST_RESULT "cl_autoreplayupload_ballchasing_testkeyresult"
#define CVAR_BALLCHASING_REPLAY_VISIBILITY "cl_autoreplayupload_ballchasing_visibility"

class Ballchasing
{
private:
	string UserAgent;
	string uploadBoundary;
	shared_ptr<CVarManagerWrapper> cvarManager = NULL;

public:
	Ballchasing(string userAgent, string uploadBoundary, shared_ptr<CVarManagerWrapper> cvarManager);
	~Ballchasing();

	shared_ptr<string> authKey = make_shared<string>("");
	shared_ptr<string> visibility = make_shared<string>("public");

	void UploadReplay(string replayPath);
	void UploadCompleted(HttpRequestObject* ctx);
	void TestAuthKey();
	void TestAuthKeyResult(HttpRequestObject* ctx);
};

