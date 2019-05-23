#pragma once

#include "HttpClient.h"
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

public:
	Ballchasing(string userAgent, string uploadBoundary, void(*Log)(void* object, string message), void(*SetVariable)(void* object, string name, string value), void* Client);
	~Ballchasing();

	shared_ptr<string> authKey = make_shared<string>("");
	shared_ptr<string> visibility = make_shared<string>("public");

	void(*Log)(void* object, string message);
	void(*SetVariable)(void* object, string name, string value);
	void* Client;

	void UploadReplay(string replayPath);
	void TestAuthKey();
};

