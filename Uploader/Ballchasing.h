#pragma once

#include <iostream>

using namespace std;

class Ballchasing
{
private:
	string UserAgent;
	string uploadBoundary;

public:
	Ballchasing(string userAgent, void(*Log)(void *object, string message), void(*NotifyUpload)(void* object, bool result), void(*NotifyAuthResult)(void *object, bool result), void * Client);
	~Ballchasing();

	shared_ptr<string> authKey = make_shared<string>("");
	shared_ptr<string> visibility = make_shared<string>("public");

	void(*Log)(void* object, string message);
	void(*NotifyAuthResult)(void* object, bool result);
	void(*NotifyUploadResult)(void* object, bool result);
	void* Client;

	void UploadReplay(string replayPath);
	void TestAuthKey();
};

