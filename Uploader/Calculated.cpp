#include "Calculated.h"

using namespace std;

Calculated::Calculated(string userAgent, string uploadBoundary, void(*log)(void* object, string message), void* client)
{
	this->UserAgent = userAgent;
	this->uploadBoundary = uploadBoundary;
	this->Log = log;
	this->Client = client;
}

void CalculatedRequestComplete(HttpRequestObject* ctx)
{
	auto calculated = (Calculated*)ctx->Requester;
	calculated->UploadCompleted(ctx);

	delete[] ctx->ReqData;
	delete[] ctx->RespData;
	delete ctx;
}

void Calculated::UploadCompleted(HttpRequestObject * ctx)
{
	Log(Client, "Calculated::UploadCompleted with status: " + to_string(ctx->Status));
}

/**
* Posts the replay file to Calculated.gg
*/
void Calculated::UploadReplay(string replayPath)
{
	if (UserAgent.empty() || replayPath.empty())
	{
		Log(Client, "Calculated::UploadReplay Parameters were empty.");
		Log(Client, "UserAgent: " + UserAgent);
		Log(Client, "ReplayPath: " + replayPath);
		return;
	}

	// Fire new thread and make request, dont't wait for response
	HttpFileUploadAsync(
		"calculated.gg",
		"api/upload",
		UserAgent,
		replayPath,
		"replays",
		"",
		uploadBoundary,
		1,
		this,
		&CalculatedRequestComplete);
}

Calculated::~Calculated()
{
}
