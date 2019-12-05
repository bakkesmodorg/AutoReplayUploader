#pragma once

#include <string>

using namespace std;

class IReplay
{
private:

public: 
	int ExportReplayCalled = 0;
	int SetReplayNameCalled = 0;

	IReplay() {}
	virtual int GetTeam0Score() { return 0; }
	virtual int GetTeam1Score() { return 0; }
	virtual void ExportReplay(string replayPath) { ExportReplayCalled++; };
	virtual void SetReplayName(string replayName) { SetReplayNameCalled++; };
};

