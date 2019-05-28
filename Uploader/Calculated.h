#pragma once

#include <iostream>

using namespace std;

class Calculated
{
private:
	string UserAgent;
	string uploadBoundary;

public:
	Calculated(string userAgent, string uploadBoundary, void(*log)(void* object, string message), void(*NotifyUploadResult)(void* object, bool result), void* client);
	~Calculated();

	void(*Log)(void* object, string message);
	void(*NotifyUploadResult)(void* object, bool result);
	void* Client;

	void UploadReplay(string replayPath);
};

