#pragma once

#include <iostream>

using namespace std;

class Calculated
{
private:
	string UserAgent;

public:
	Calculated(string userAgent, void(*log)(void* object, string message), void(*NotifyUploadResult)(void* object, bool result), void* client);
	~Calculated();

	shared_ptr<string> visibility = make_shared<string>("PUBLIC");

	void(*Log)(void* object, string message);
	void(*NotifyUploadResult)(void* object, bool result);
	void* Client;

	void UploadReplay(string replayPath, string replayFileName, string playerId);
};

