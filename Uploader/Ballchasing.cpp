#include "Ballchasing.h"

#include "HttpClient.h"
#include <sstream>

using namespace std;

Ballchasing::Ballchasing(string userAgent, void(*Log)(void *object, string message), void(*NotifyUploadResult)(void* object, bool result), void(*NotifyAuthResult)(void *object, bool result), void * Client)
{
	this->UserAgent = userAgent;
	this->Log = Log;
	this->NotifyUploadResult = NotifyUploadResult;
	this->NotifyAuthResult = NotifyAuthResult;
	this->Client = Client;
}

void BallchasingRequestComplete(PostFileRequest* ctx)
{
	auto ballchasing = (Ballchasing*)ctx->Requester;

	if (ctx->RequestId == 1)
	{
		ballchasing->Log(ballchasing->Client, "Ballchasing::UploadCompleted with status: " + to_string(ctx->Status));
		if (ctx->message.size() > 0)
		{
			ballchasing->Log(ballchasing->Client, ctx->message);
		}
		ballchasing->NotifyUploadResult(ballchasing->Client, (ctx->Status >= 200 && ctx->Status < 300));

		DeleteFile(ctx->FilePath.c_str());

		delete ctx;
	}
}

void BallchasingRequestComplete(GetRequest* ctx)
{
	auto ballchasing = (Ballchasing*)ctx->Requester;

	if (ctx->RequestId == 2)
	{
		ballchasing->Log(ballchasing->Client, "Ballchasing::AuthTest completed with status: " + to_string(ctx->Status));
		ballchasing->NotifyAuthResult(ballchasing->Client, ctx->Status == 200);

		delete ctx;
	}
}

void Ballchasing::UploadReplay(string replayPath, string replayFileName)
{
	if (UserAgent.empty() || authKey->empty() || visibility->empty() || replayPath.empty() || replayFileName.empty())
	{
		Log(Client, "Ballchasing::UploadReplay Parameters were empty.");
		Log(Client, "UserAgent: " + UserAgent);
		Log(Client, "ReplayPath: " + replayPath);
		Log(Client, "AuthKey: " + *authKey);
		Log(Client, "Visibility: " + *visibility);
		Log(Client, "ReplayFileName: " + replayFileName);
		return;
	}

	string destPath = "./bakkesmod/data/ballchasing/" + replayFileName;
	CreateDirectory("./bakkesmod/data/ballchasing", NULL);
	CopyFile(replayPath.c_str(), destPath.c_str(), FALSE);

	PostFileRequest *request = new PostFileRequest();
	request->Url = AppendGetParams("https://ballchasing.com/api/v2/upload", { {"visibility", *visibility} });
    request->FilePath = destPath;
	request->ParamName = "file";
	request->Headers.push_back("Authorization: " + *authKey);
	request->Headers.push_back("UserAgent: " + UserAgent);
	request->RequestComplete = &BallchasingRequestComplete;
	request->RequestId = 1;
	request->Requester = this;
	request->message = "";

	PostFileAsync(request);
}

/**
* Tests the authorization key for Ballchasing.com
*/
void Ballchasing::TestAuthKey()
{
	GetRequest *request = new GetRequest();
	request->Url = "https://ballchasing.com/api/";
	request->Headers.push_back("Authorization: " + *authKey);
	request->Headers.push_back("UserAgent: " + UserAgent);
	request->RequestComplete = &BallchasingRequestComplete;
	request->RequestId = 2;
	request->Requester = this;

	GetAsync(request);
}

Ballchasing::~Ballchasing()
{
}