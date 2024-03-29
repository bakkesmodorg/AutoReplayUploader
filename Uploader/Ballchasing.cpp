#include "Ballchasing.h"

#include "HttpClient.h"
#include <sstream>


Ballchasing::Ballchasing(std::string userAgent, void(*Log)(void *object, std::string message), void(*NotifyUploadResult)(void* object, bool result), void(*NotifyAuthResult)(void *object, bool result), void * Client)
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
		ballchasing->Log(ballchasing->Client, "Ballchasing::UploadCompleted with status: " + std::to_string(ctx->Status));
		if (ctx->Message.size() > 0)
		{
			ballchasing->Log(ballchasing->Client, ctx->Message);
		}
		if (ctx->ResponseBody.size() > 0)
		{
			ballchasing->Log(ballchasing->Client, ctx->ResponseBody);
		}
		ballchasing->NotifyUploadResult(ballchasing->Client, (ctx->Status >= 200 && ctx->Status < 300));

		DeleteFile(ctx->FilePath.c_str());

		delete ctx;
	}
}

void BallchasingRequestComplete(PostJsonRequest* ctx)
{
	auto ballchasing = (Ballchasing*)ctx->Requester;

	if (ctx->RequestId == 1)
	{
		ballchasing->Log(ballchasing->Client, "Ballchasing::UploadMMRComplete with status: " + std::to_string(ctx->Status));
		if (ctx->Message.size() > 0)
		{
			ballchasing->Log(ballchasing->Client, ctx->Message);
		}
		if (ctx->ResponseBody.size() > 0)
		{
			ballchasing->Log(ballchasing->Client, ctx->ResponseBody);
		}
		ballchasing->NotifyUploadResult(ballchasing->Client, (ctx->Status >= 200 && ctx->Status < 300));

		delete ctx;
	}
}

void BallchasingRequestComplete(GetRequest* ctx)
{
	auto ballchasing = (Ballchasing*)ctx->Requester;

	if (ctx->RequestId == 2)
	{
		ballchasing->Log(ballchasing->Client, "Ballchasing::AuthTest completed with status: " + std::to_string(ctx->Status));
		ballchasing->NotifyAuthResult(ballchasing->Client, ctx->Status == 200);

		delete ctx;
	}
}

void Ballchasing::UploadReplay(std::string replayPath)
{
	if (!IsValid() || replayPath.empty())
	{
		Log(Client, "ReplayPath: " + replayPath);
		return;
	}

	std::string destPath = "./bakkesmod/data/ballchasing/temp.replay";
	CreateDirectory("./bakkesmod/data/ballchasing", NULL);
	bool resultOfCopy = CopyFile(replayPath.c_str(), destPath.c_str(), FALSE);

	Log(Client, "ReplayPath: " + replayPath);
	Log(Client, "DestPath: " + destPath);
	Log(Client, "File copy success: " + std::string(resultOfCopy ? "true" : "false"));

	PostFileRequest *request = new PostFileRequest();
	request->Url = AppendGetParams("https://ballchasing.com/api/v2/upload", { {"visibility", *visibility} });
    request->FilePath = destPath;
	request->ParamName = "file";
	request->Headers.push_back("Authorization: " + *authKey);
	request->Headers.push_back("UserAgent: " + UserAgent);
	request->RequestComplete = &BallchasingRequestComplete;
	request->RequestId = 1;
	request->Requester = this;
	request->Message = "";

	PostFileAsync(request);
}

void Ballchasing::UploadMMr(MMRData data)
{
	if (!IsValid())
	{
		return;
	}

	try {
		json json_body = data;
		std::string body = json_body.dump();

		PostJsonRequest* request = new PostJsonRequest();
		request->Url = "https://ballchasing.com/api/v1/mmr";
		request->Headers.push_back("Authorization: " + *authKey);
		request->Headers.push_back("UserAgent: " + UserAgent);
		request->body = body;
		request->RequestComplete = &BallchasingRequestComplete;
		request->RequestId = 1;
		request->Requester = this;
		request->Message = "";

		PostJsonAsync(request);
	}
	catch (const std::exception & e) {
		Log(Client,  e.what());
		return;
	}

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

bool Ballchasing::IsValid()
{
	if (UserAgent.empty() || authKey->empty() || visibility->empty() )
	{
		Log(Client, "Ballchasing::UploadReplay Parameters were empty.");
		Log(Client, "UserAgent: " + UserAgent);
		Log(Client, "AuthKey: " + *authKey);
		Log(Client, "Visibility: " + *visibility);
		return false;
	}
	return true;
}

Ballchasing::~Ballchasing()
{
}