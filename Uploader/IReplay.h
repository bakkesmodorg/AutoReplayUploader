#pragma once

#include <string>

using namespace std;

class IReplay
{
private:

public: 
	IReplay() {}
	virtual int GetTeam0Score() { return 0; }
	virtual int GetTeam1Score() { return 0; }
	virtual void ExportReplay(string replayPath) {};
	virtual void SetReplayName(string replayName) {};
};

