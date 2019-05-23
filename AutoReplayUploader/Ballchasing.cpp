#include "Ballchasing.h"
#include "Utils.h"

#include <sstream>

using namespace std;

Ballchasing::Ballchasing(string userAgent, string uploadBoundary, shared_ptr<CVarManagerWrapper> cvarManager)
{
	this->UserAgent = userAgent;
	this->uploadBoundary = uploadBoundary;
	this->cvarManager = cvarManager;
}

void BallchasingRequestComplete(HttpRequestObject* ctx)
{
	if (ctx->RequestId == 1)
	{
		auto ballchasing = (Ballchasing*)ctx->Requester;
		ballchasing->UploadCompleted(ctx);

		delete[] ctx->ReqData;
		delete[] ctx->RespData;
		delete ctx;
	}
	else if(ctx->RequestId == 2)
	{
		auto ballchasing = (Ballchasing*)ctx->Requester;
		ballchasing->TestAuthKeyResult(ctx);

		delete[] ctx->RespData;
		delete ctx;
	}
}

void Ballchasing::TestAuthKeyResult(HttpRequestObject* ctx)
{
	string result = ctx->Status == 200 ? "Auth key correct!" : "Invalid auth key!";
	cvarManager->getCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT).setValue(result);
}

void Ballchasing::UploadCompleted(HttpRequestObject* ctx)
{
	cvarManager->log("Ballchasing::UploadCompleted with status: " + to_string(ctx->Status));
}

void Ballchasing::UploadReplay(string replayPath)
{
	if (UserAgent.empty() || authKey->empty() || visibility->empty() || replayPath.empty())
	{
		cvarManager->log("Ballchasing::UploadReplay Parameters were empty.");
		cvarManager->log("UserAgent: " + UserAgent);
		cvarManager->log("ReplayPath: " + replayPath);
		cvarManager->log("AuthKey: " + *authKey);
		cvarManager->log("Visibility: " + *visibility);
		return;
	}

	// Get Replay file bytes to upload
	auto bytes = GetFileBytes(replayPath);

	// Construct headers
	stringstream headers;
	headers << "Authorization: " << *authKey << "\r\n";
	headers << "Content-Type: multipart/form-data;boundary=" << uploadBoundary;
	auto header_str = headers.str();

	// Construct body
	stringstream body;
	body << "--" << uploadBoundary << "\r\n";
	body << "Content-Disposition: form-data; name=\"file\"; filename=\"autosavedreplay.replay\"" << "\r\n";
	body << "Content-Type: application/form-data" << "\r\n";
	body << "\r\n";
	body << string(bytes.begin(), bytes.end());
	body << "\r\n";
	body << "--" << uploadBoundary << "--" << "\r\n";

	// Convert body to vector of bytes instead of using str() which may have trouble with null termination chars
	vector<uint8_t> buffer;
	const string& str = body.str();
	buffer.insert(buffer.end(), str.begin(), str.end());

	// Copy vector to char* for upload
	char *reqData = CopyToCharPtr(buffer);

	// Append get parmeters to path
	string path = AppendGetParams("api/upload", { {"visibility", *visibility} });

	// Setup Http Request context
	HttpRequestObject* ctx = new HttpRequestObject();
	ctx->RequestId = 1;
	ctx->Requester = this;
	ctx->Headers = header_str;
	ctx->Server = "ballchasing.com";
	ctx->Page = path;
	ctx->Method = "POST";
	ctx->UserAgent = UserAgent;
	ctx->Port = INTERNET_DEFAULT_HTTPS_PORT;
	ctx->ReqData = reqData;
	ctx->ReqDataSize = buffer.size();
	ctx->RespData = new char[4096];
	ctx->RespDataSize = 4096;
	ctx->RequestComplete = &BallchasingRequestComplete;
	ctx->Flags = INTERNET_FLAG_SECURE;

	// Fire new thread and make request, dont't wait for response
	HttpRequestAsync(ctx);
}

/**
* Tests the authorization key for Ballchasing.com
*/
void Ballchasing::TestAuthKey()
{
	// Construct headers
	stringstream headers;
	headers << "Authorization: " << *authKey;
	auto header_str = headers.str();

	HttpRequestObject* ctx = new HttpRequestObject();
	ctx->RequestId = 2;
	ctx->Requester = this;
	ctx->Headers = header_str;
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