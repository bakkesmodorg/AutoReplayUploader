#pragma once

#include <iostream>
#include <chrono>
#include "IReplayUploader.h"

using namespace std;

#define BALLCHASING_DEFAULT_REPLAY_VISIBILITY "public"
#define BALLCHASING_DEFAULT_AUTH_TEST_RESULT "Untested"
#define BALLCHASING_DEFAULT_AUTH_KEY ""

class Ballchasing : IReplayUploader
{
private:
	string UserAgent;
	string uploadBoundary;
	chrono::time_point<chrono::steady_clock> pluginLoadTime;

public:
	Ballchasing(string userAgent, void(*Log)(void *object, string message), void(*NotifyUpload)(void* object, bool result), void(*NotifyAuthResult)(void *object, bool result), void * Client);
	~Ballchasing();

	shared_ptr<string> authKey = make_shared<string>(BALLCHASING_DEFAULT_AUTH_KEY);
	shared_ptr<string> visibility = make_shared<string>(BALLCHASING_DEFAULT_REPLAY_VISIBILITY);

	void(*Log)(void* object, string message);
	void(*NotifyAuthResult)(void* object, bool result);
	void(*NotifyUploadResult)(void* object, bool result);
	void* Client;

	void UploadReplay(string replayPath, string playerId);
	void TestAuthKey();
	void OnBallChasingAuthKeyChanged(string& oldVal);
};

