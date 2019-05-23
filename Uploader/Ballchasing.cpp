#include "Ballchasing.h"
#include <sstream>

using namespace std;

Ballchasing::Ballchasing(string userAgent, string uploadBoundary, void(*Log)(void* object, string message), void(*SetVariable)(void* object, string name, string value), void* Client)
{
	this->UserAgent = userAgent;
	this->uploadBoundary = uploadBoundary;
	this->Log = Log;
	this->SetVariable = SetVariable;
	this->Client = Client;
}

void BallchasingRequestComplete(HttpRequestObject* ctx)
{
	if (ctx->RequestId == 1)
	{
		auto ballchasing = (Ballchasing*)ctx->Requester;
		ballchasing->Log(ballchasing->Client, "Ballchasing::UploadCompleted with status: " + to_string(ctx->Status));
		
		delete[] ctx->ReqData;
		delete[] ctx->RespData;
		delete ctx;
	}
	else if(ctx->RequestId == 2)
	{
		auto ballchasing = (Ballchasing*)ctx->Requester;
		string result = ctx->Status == 200 ? "Auth key correct!" : "Invalid auth key!";
		ballchasing->SetVariable(ballchasing->Client, CVAR_BALLCHASING_AUTH_TEST_RESULT, result);

		delete[] ctx->RespData;
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

	// Fire new thread and make request, dont't wait for response
	HttpFileUploadAsync(
		"ballchasing.com",
		AppendGetParams("api/upload", { {"visibility", *visibility} }),
		UserAgent,
		replayPath,
		"file",
		"Authorization: " + *authKey,
		uploadBoundary,
		1,
		this,
		&BallchasingRequestComplete);
}

/**
* Tests the authorization key for Ballchasing.com
*/
void Ballchasing::TestAuthKey()
{
	HttpRequestObject* ctx = new HttpRequestObject();
	ctx->RequestId = 2;
	ctx->Requester = this;
	ctx->Headers = "Authorization: " + *authKey;
	ctx->Server = "ballchasing.com";
	ctx->Page = "api/";
	ctx->Method = "GET";
	ctx->UserAgent = UserAgent;
	ctx->Port = INTERNET_DEFAULT_HTTPS_PORT;
	ctx->RespData = new char[4096];
	ctx->RespDataSize = 4096;
	ctx->RequestComplete = &BallchasingRequestComplete;
	ctx->Flags = INTERNET_FLAG_SECURE;

	// Fire new thread and make request, dont't wait for response
	HttpRequestAsync(ctx);
}

Ballchasing::~Ballchasing()
{
}