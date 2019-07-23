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
		ballchasing->NotifyUploadResult(ballchasing->Client, (ctx->Status >= 200 && ctx->Status < 300));
		
		delete ctx;
	}
	else if(ctx->RequestId == 2)
	{
		ballchasing->Log(ballchasing->Client, "Ballchasing::AuthTest completed with status: " + to_string(ctx->Status));
		ballchasing->NotifyAuthResult(ballchasing->Client, ctx->Status == 200);

		delete ctx;
	}
}

void Ballchasing::UploadReplay(string replayPath)
{
	if (UserAgent.empty() || authKey->empty() || visibility->empty() || replayPath.empty())
	{
		Log(Client, "Ballchasing::UploadReplay Parameters were empty.");
		Log(Client, "UserAgent: " + UserAgent);
		Log(Client, "ReplayPath: " + replayPath);
		Log(Client, "AuthKey: " + *authKey);
		Log(Client, "Visibility: " + *visibility);
		return;
	}

	PostFileRequest *request = new PostFileRequest();
	request->Url = AppendGetParams("https://ballchasing.com/api/v2/upload", { {"visibility", *visibility} });
	request->FilePath = replayPath;
	request->ParamName = "file";
	request->Headers.push_back("Authorization: " + *authKey);
	request->Headers.push_back("UserAgent: " + UserAgent);
	request->RequestComplete = &BallchasingRequestComplete;
	request->RequestId = 1;
	request->Requester = this;

	PostFileAsync(request);
}

/**
* Tests the authorization key for Ballchasing.com
*/
void Ballchasing::TestAuthKey()
{
	//HttpRequestObject* ctx = new HttpRequestObject();
	//ctx->RequestId = 2;
	//ctx->Requester = this;
	//ctx->Headers = "Authorization: " + *authKey;
	//ctx->Server = "ballchasing.com";
	//ctx->Page = "api/";
	//ctx->Method = "GET";
	//ctx->UserAgent = UserAgent;
	//ctx->Port = INTERNET_DEFAULT_HTTPS_PORT;
	//ctx->RespData = new char[4096];
	//ctx->RespDataSize = 4096;
	//ctx->RequestComplete = &BallchasingRequestComplete;
	//ctx->Flags = INTERNET_FLAG_SECURE;

	//// Fire new thread and make request, dont't wait for response
	//HttpRequestAsync(ctx);
}

Ballchasing::~Ballchasing()
{
}