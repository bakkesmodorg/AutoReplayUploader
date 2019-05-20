#include "Logger.h"
#include "plog/Log.h"


Logger::Logger(shared_ptr<CVarManagerWrapper> cvarManager)
{
	this->cvarManager = cvarManager;
}

void Logger::Log(string message)
{
	LOG(plog::debug) << message;

	if(cvarManager != NULL)
		cvarManager->log(message);
}


Logger::~Logger()
{
}
