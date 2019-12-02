#include "Calculated.h"

#include "HttpClient.h"

using namespace std;

Calculated::Calculated(string userAgent, void(*log)(void* object, string message), void(*NotifyUploadResult)(void* object, bool result), void* client)
{
	this->UserAgent = userAgent;
	this->Log = log;
	this->NotifyUploadResult = NotifyUploadResult;
	this->Client = client;
}

void CalculatedRequestComplete(PostFileRequest* ctx)
{
	auto calculated = (Calculated*)ctx->Requester;

	calculated->Log(calculated->Client, "Calculated::UploadCompleted with status: " + to_string(ctx->Status));
	calculated->NotifyUploadResult(calculated->Client, (ctx->Status >= 200 && ctx->Status < 300));

	if (ctx->message.size() > 0)
	{
		calculated->Log(calculated->Client, ctx->message);
	}

	DeleteFile(ctx->FilePath.c_str());

	delete ctx;
}

/**
* Posts the replay file to Calculated.gg
*/
void Calculated::UploadReplay(string replayPath, string replayFileName, string playerId)
{
	if (UserAgent.empty() || replayPath.empty() || replayFileName.empty())
	{
		Log(Client, "Calculated::UploadReplay Parameters were empty.");
		Log(Client, "UserAgent: " + UserAgent);
		Log(Client, "ReplayPath: " + replayPath);
		Log(Client, "ReplayFileName: " + replayFileName);
		return;
	}

	string path = AppendGetParams("https://calculated.gg/api/upload", { {"player_id", playerId}, {"visibility", *visibility} });

	string destPath = "./bakkesmod/data/calculated/" + replayFileName + ".replay";
	CreateDirectory("./bakkesmod/data/calculated", NULL);
	CopyFile(replayPath.c_str(), destPath.c_str(), FALSE);

	PostFileRequest *request = new PostFileRequest();
	request->Url = path;
	request->FilePath = destPath;
	request->ParamName = "replays";
	request->Headers.push_back("UserAgent: " + UserAgent);
	request->RequestComplete = &CalculatedRequestComplete;
	request->RequestId = 1;
	request->Requester = this;
	request->message = "";

	PostFileAsync(request);
}

Calculated::~Calculated()
{
}
