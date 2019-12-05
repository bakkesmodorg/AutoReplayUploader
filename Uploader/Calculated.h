#pragma once

#include <iostream>
#include "IReplayUploader.h"

using namespace std;

#define CALCULATED_DEFAULT_REPLAY_VISIBILITY "DEFAULT"

class Calculated : IReplayUploader
{
private:
	string UserAgent;

public:
	Calculated(string userAgent, void(*log)(void* object, string message), void(*NotifyUploadResult)(void* object, bool result), void* client);
	~Calculated();

	shared_ptr<string> visibility = make_shared<string>(CALCULATED_DEFAULT_REPLAY_VISIBILITY);

	void(*Log)(void* object, string message);
	void(*NotifyUploadResult)(void* object, bool result);
	void* Client;

	void UploadReplay(string replayPath, string playerId);
};

