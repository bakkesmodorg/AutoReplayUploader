#pragma once

#include <iostream>
#include <filesystem>
using namespace std;

class Calculated
{
private:
	string UserAgent;

public:
	Calculated(string userAgent, void(*log)(void* object, string message), void(*NotifyUploadResult)(void* object, bool result), void* client);
	~Calculated();

	shared_ptr<string> visibility = make_shared<string>("DEFAULT");

	void(*Log)(void* object, string message);
	void(*NotifyUploadResult)(void* object, bool result);
	void* Client;

	void UploadReplay(std::filesystem::path startPath, std::filesystem::path replayPath, string playerId);
};

