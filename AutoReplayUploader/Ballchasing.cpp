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
	auto ballchasing = (Ballchasing*)ctx->Requester;
	ballchasing->UploadCompleted(ctx);

	delete[] ctx->ReqData;
	delete[] ctx->RespData;
	delete ctx;
}

void Ballchasing::UploadCompleted(HttpRequestObject* ctx)
{
	cvarManager->log("Ballchasing::UploadCompleted with status: " + to_string(ctx->Status));
}

void Ballchasing::UploadReplay(string replayPath, string authKey, string visibility)
{
	if (UserAgent.empty() || authKey.empty() || visibility.empty() || replayPath.empty())
	{
		cvarManager->log("Ballchasing::UploadReplay Parameters were empty.");
		cvarManager->log("UserAgent: " + UserAgent);
		cvarManager->log("ReplayPath: " + replayPath);
		cvarManager->log("AuthKey: " + authKey);
		cvarManager->log("Visibility: " + visibility);
		return;
	}

	// Get Replay file bytes to upload
	auto bytes = GetFileBytes(replayPath);

	// Construct headers
	stringstream headers;
	headers << "Authorization: " << authKey << "\r\n";
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
	string path = AppendGetParams("api/upload", { {"visibility", visibility} });

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
bool Ballchasing::TestAuthKey(string authKey)
{
	return false;
	/*HTTPRequestHandle hdl = steamHTTPInstance->CreateHTTPRequest(k_EHTTPMethodGET, "https://ballchasing.com/api/");
	SteamAPICall_t* callHandle = NULL;
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "User-Agent", userAgent.c_str());

	std::string authKey = cvarManager->getCvar(CVAR_BALLCHASING_AUTH_KEY).getStringValue();
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "Authorization", authKey.c_str());

	AuthKeyCheckUploadData* uploadData = new AuthKeyCheckUploadData(cvarManager);
	uploadData->requestHandle = hdl;
	uploadData->requester = this;
	steamHTTPInstance->SendHTTPRequest(uploadData->requestHandle, &uploadData->apiCall);
	uploadData->requestCompleteCallback.Set(uploadData->apiCall, uploadData, &FileUploadData::OnRequestComplete);

	fileUploadsInProgress.push_back(uploadData);
	CheckFileUploadProgress(gameWrapper.get());*/
}

Ballchasing::~Ballchasing()
{
}