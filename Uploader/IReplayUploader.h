#pragma once
#include <string>

using namespace std;

class IReplayUploader
{
private:

public:
	IReplayUploader() {}
	virtual void UploadReplay(string replayPath, string playerId) {}

};