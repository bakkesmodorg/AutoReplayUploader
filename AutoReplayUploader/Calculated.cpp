#include "Calculated.h"
#include "Utils.h"

#include <sstream>

using namespace std;

Calculated::Calculated(string userAgent, string uploadBoundary, shared_ptr<CVarManagerWrapper> cvarManager)
{
	this->UserAgent = userAgent;
	this->UploadBoundary = uploadBoundary;
	this->cvarManager = cvarManager;
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
	cvarManager->log("Calculated::UploadCompleted with status: " + to_string(ctx->Status));
}

/**
* Posts the replay file to Calculated.gg
*/
void Calculated::UploadReplay(string replayPath)
{
	if (UserAgent.empty() || replayPath.empty())
	{
		cvarManager->log("Calculated::UploadReplay Parameters were empty.");
		cvarManager->log("UserAgent: " + UserAgent);
		cvarManager->log("ReplayPath: " + replayPath);
		return;
	}

	// Get Replay file bytes to upload
	auto bytes = GetFileBytes(replayPath);

	// Construct headers
	stringstream headers;
	headers << "User-Agent: " << UserAgent << "\r\n";
	headers << "Content-Type: multipart/form-data;boundary=" << UploadBoundary;
	auto header_str = headers.str();

	// Construct body
	stringstream body;
	body << "--" << UploadBoundary << "\r\n";
	body << "Content-Disposition: form-data; name=\"replays\"; filename=\"autosavedreplay.replay\"" << "\r\n";
	body << "Content-Type: application/octet-stream" << "\r\n";
	body << "\r\n";
	body << string(bytes.begin(), bytes.end());
	body << "\r\n";
	body << "--" << UploadBoundary << "--" << "\r\n";
	
	// Convert body to vector of bytes instead of using str() which may have trouble with null termination chars
	vector<uint8_t> buffer;
	const string& str = body.str();
	buffer.insert(buffer.end(), str.begin(), str.end());

	// Copy vector to char* for upload
	char *reqData = CopyToCharPtr(buffer);

	// Setup Http Request context
	HttpRequestObject* ctx = new HttpRequestObject();
	ctx->RequestId = 1;
	ctx->Requester = this;
	ctx->Headers = header_str;
	ctx->Server = "calculated.gg";
	ctx->Page = "api/upload";
	ctx->Method = "POST";
	ctx->UserAgent = UserAgent;
	ctx->Port = INTERNET_DEFAULT_HTTPS_PORT;
	ctx->ReqData = reqData;
	ctx->ReqDataSize = buffer.size();
	ctx->RespData = new char[4096];
	ctx->RespDataSize = 4096;
	ctx->RequestComplete = &CalculatedRequestComplete;
	ctx->Flags = INTERNET_FLAG_SECURE;

	// Fire new thread and make request, dont't wait for response
	HttpRequestAsync(ctx);
}

Calculated::~Calculated()
{
}
