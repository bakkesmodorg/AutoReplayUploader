#pragma once

#include <iostream>


class Calculated
{
private:
	std::string UserAgent;

public:
	Calculated(std::string userAgent, void(*log)(void* object, std::string message), void(*NotifyUploadResult)(void* object, bool result), void* client);
	~Calculated();

	std::shared_ptr<std::string> visibility = std::make_shared<std::string>("DEFAULT");

	void(*Log)(void* object, std::string message);
	void(*NotifyUploadResult)(void* object, bool result);
	void* Client;

	void UploadReplay(std::string replayPath, std::string playerId);
};

