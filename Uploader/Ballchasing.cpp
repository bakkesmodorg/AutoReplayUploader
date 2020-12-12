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
		if (ctx->Message.size() > 0)
		{
			ballchasing->Log(ballchasing->Client, ctx->Message);
		}
		if (ctx->ResponseBody.size() > 0)
		{
			ballchasing->Log(ballchasing->Client, ctx->ResponseBody);
		}
		ballchasing->NotifyUploadResult(ballchasing->Client, (ctx->Status >= 200 && ctx->Status < 300));

		std::filesystem::remove(ctx->FilePath);

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

void Ballchasing::UploadReplay(std::filesystem::path startPath, std::filesystem::path replayPath)
{
	if (UserAgent.empty() || authKey->empty() || visibility->empty() || replayPath.empty())
	{
		Log(Client, "Ballchasing::UploadReplay Parameters were empty.");
		Log(Client, "UserAgent: " + UserAgent);
		Log(Client, "ReplayPath: " + replayPath.string());
		Log(Client, "AuthKey: " + *authKey);
		Log(Client, "Visibility: " + *visibility);
		return;
	}
	try
	{
		if (!std::filesystem::exists(replayPath))
		{
			Log(Client, "Replay path doesn't exist? " + replayPath.string());
			return;
		}
		std::filesystem::path destPath = startPath / "data/ballchasing/temp.replay";
		std::filesystem::path tempFolder = startPath / "data/ballchasing/";
		if (!std::filesystem::exists(tempFolder))
		{
			std::filesystem::create_directory(tempFolder);
		}
		if (std::filesystem::exists(destPath))
		{
			Log(Client, "Destination path exists, removing " + destPath.string());
			std::filesystem::remove(destPath);
		}

		std::filesystem::copy(replayPath, destPath);

		Log(Client, "ReplayPath: " + replayPath.string());
		Log(Client, "DestPath: " + destPath.string());
		Log(Client, "File copy success: " + std::string(std::filesystem::exists(destPath) ? "true" : "false"));

		PostFileRequest* request = new PostFileRequest();
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
	catch (std::exception e)
	{
		Log(Client, "AutoreplayUploader ERR: " + std::string(e.what()));
	}
	catch (...)
	{
		Log(Client, "BAD AutoreplayUploader ERR!");
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

Ballchasing::~Ballchasing()
{
}