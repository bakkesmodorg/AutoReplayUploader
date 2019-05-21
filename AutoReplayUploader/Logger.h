#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"

using namespace std;

class Logger
{
private:

public:
	shared_ptr<CVarManagerWrapper> cvarManager = NULL;
	Logger(shared_ptr<CVarManagerWrapper> cvarManager);
	void Log(string message);
	~Logger();
};

