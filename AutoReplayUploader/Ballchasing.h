#pragma once

#include "Wininet.h"
#include "Logger.h"

#include <iostream>
#include <vector>

using namespace std;

class Ballchasing
{
private:
	string userAgent;
	string uploadBoundary;
	char uploadReplayResult[4096];
	Logger* logger;

public:
	Ballchasing(string userAgent, string uploadBoundary, Logger* logger);
	~Ballchasing();

	bool UploadReplay(string replayPath, string authKey, string visibility);
	void UploadCompleted(HttpRequestObject* ctx);
	bool TestAuthKey(string authKey);
};

