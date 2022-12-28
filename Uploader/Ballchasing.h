#pragma once

#include <iostream>
#include "MMRData.h"

class Ballchasing
{
private:
	std::string UserAgent;
	std::string uploadBoundary;

public:
	Ballchasing(std::string userAgent, void(*Log)(void *object, std::string message), void(*NotifyUpload)(void* object, bool result), void(*NotifyAuthResult)(void *object, bool result), void * Client);
	~Ballchasing();

	std::shared_ptr<std::string> authKey = std::make_shared<std::string>("");
	std::shared_ptr<std::string> visibility = std::make_shared<std::string>("public");
	
	void(*Log)(void* object, std::string message);
	void(*NotifyAuthResult)(void* object, bool result);
	void(*NotifyUploadResult)(void* object, bool result);
	void* Client;

	void UploadReplay(std::string replayPath);
	void UploadMMr(MMRData);
	void TestAuthKey();
	bool IsValid();
};

