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
	if (ctx->Message.size() > 0)
	{
		calculated->Log(calculated->Client, ctx->Message);
	}
	if (ctx->ResponseBody.size() > 0)
	{
		calculated->Log(calculated->Client, ctx->ResponseBody);
	}
	calculated->NotifyUploadResult(calculated->Client, (ctx->Status >= 200 && ctx->Status < 300));

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

	string path = AppendGetParams("https://upload.calculated.gg/parse_replay", { {"player_id", playerId}, {"visibility", *visibility} });

	string destPath = "./bakkesmod/data/calculated/" + replayFileName + ".replay";
	CreateDirectory("./bakkesmod/data/calculated", NULL);
	bool resultOfCopy = CopyFile(replayPath.c_str(), destPath.c_str(), FALSE);

	Log(Client, "ReplayPath: " + replayPath);
	Log(Client, "ReplayFileName: " + replayFileName);
	Log(Client, "DestPath: " + destPath);
	Log(Client, "File copy success: " + std::string(resultOfCopy ? "true" : "false"));

	PostFileRequest *request = new PostFileRequest();
	request->Url = path;
	request->FilePath = destPath;
	request->ParamName = "replays";
	request->Headers.push_back("UserAgent: " + UserAgent);
	request->RequestComplete = &CalculatedRequestComplete;
	request->RequestId = 1;
	request->Requester = this;
	request->Message = "";

	PostFileAsync(request);
}

Calculated::~Calculated()
{
}
