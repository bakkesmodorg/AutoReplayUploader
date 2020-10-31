#include "Calculated.h"

#include "HttpClient.h"

using namespace std;

#define CALCULATED_ENDPOINT_URL  "https://us-east1-calculatedgg-217303.cloudfunctions.net/queue_replay"

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

	std::filesystem::remove(ctx->FilePath);

	delete ctx;
}

/**
* Posts the replay file to Calculated.gg
*/
void Calculated::UploadReplay(std::filesystem::path startPath, std::filesystem::path replayPath, string playerId)
{
	if (UserAgent.empty() || replayPath.empty())
	{
		Log(Client, "Calculated::UploadReplay Parameters were empty.");
		Log(Client, "UserAgent: " + UserAgent);
		Log(Client, "ReplayPath: " + replayPath.string());
		return;
	}

	string path = AppendGetParams(CALCULATED_ENDPOINT_URL, { {"player_id", playerId}, {"visibility", *visibility} });
	Log(Client, "ReplayPath: " + replayPath.string());
	
	std::filesystem::path destPath = startPath / "data/calculated/temp.replay";
	Log(Client, "DestPath: " + destPath.string());
	std::filesystem::path tempFolder = startPath / "data/calculated/";
	if (!std::filesystem::exists(tempFolder))
	{
		std::filesystem::create_directory(tempFolder);
	}

	std::filesystem::copy(replayPath, destPath);

	
	Log(Client, "File copy success: " + std::string(std::filesystem::exists(destPath) ? "true" : "false"));

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
