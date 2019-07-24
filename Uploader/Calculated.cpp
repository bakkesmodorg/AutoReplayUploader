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

	delete ctx;
}

/**
* Posts the replay file to Calculated.gg
*/
void Calculated::UploadReplay(string replayPath, string playerId)
{
	if (UserAgent.empty() || replayPath.empty())
	{
		Log(Client, "Calculated::UploadReplay Parameters were empty.");
		Log(Client, "UserAgent: " + UserAgent);
		Log(Client, "ReplayPath: " + replayPath);
		return;
	}

	string path = AppendGetParams("https://calculated.gg/api/upload", { {"player_id", playerId}, {"visibility", *visibility} });

	PostFileRequest *request = new PostFileRequest();
	request->Url = path;
	request->FilePath = replayPath;
	request->ParamName = "replays";
	request->Headers.push_back("UserAgent: " + UserAgent);
	request->RequestComplete = &CalculatedRequestComplete;
	request->RequestId = 1;
	request->Requester = this;

	PostFileAsync(request);
}

Calculated::~Calculated()
{
}
