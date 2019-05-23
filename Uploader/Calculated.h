#pragma once

#include "HttpClient.h"
#include <iostream>

using namespace std;

class Calculated
{
private:
	string UserAgent;
	string uploadBoundary;
	void(*Log)(void* object, string message);
	void* Client;

public:
	Calculated(string userAgent, string uploadBoundary, void(*log)(void* object, string message), void* client);
	~Calculated();

	void UploadReplay(string replayPath);
	void UploadCompleted(HttpRequestObject* ctx);
};

