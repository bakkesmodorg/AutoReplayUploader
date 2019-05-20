#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"

using namespace std;

class Logger
{
private:
	shared_ptr<CVarManagerWrapper> cvarManager = NULL;

public:
	Logger(shared_ptr<CVarManagerWrapper> cvarManager);
	void Log(string message);
	~Logger();
};

