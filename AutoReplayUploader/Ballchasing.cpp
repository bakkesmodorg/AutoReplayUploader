#include "Ballchasing.h"
#include "Utils.h"

#include <sstream>
#include <fstream>

Ballchasing::Ballchasing(string userAgent, string uploadBoundary, Logger* logger)
{
	this->userAgent = userAgent;
	this->uploadBoundary = uploadBoundary;
	this->logger = logger;
}

void RequestComplete(HttpRequestObject* ctx)
{
	auto ballchasing = (Ballchasing*)ctx->Requester;
	ballchasing->UploadCompleted(ctx);

	delete[] ctx->ReqData;
	delete[] ctx->RespData;
	delete ctx;
}

void Ballchasing::UploadCompleted(HttpRequestObject* ctx)
{
	logger->Log("UploadCompleted with status: " + to_string(ctx->Status));
}

bool Ballchasing::UploadReplay(string replayPath, string authKey, string visibility)
{
	if (userAgent.empty() || authKey.empty() || visibility.empty() || replayPath.empty())
	{
		logger->Log("Ballchasing::UploadReplay Parameters were empty.");
		return false;
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
	auto body_str = body.str();
	char *reqData = new char[body_str.length() + 1];
	strcpy(reqData, body_str.c_str());

	string path = AppendGetParams("api/upload", { {"visibility", visibility} });

	HttpRequestObject* request = new HttpRequestObject();
	request->RequestId = 1;
	request->Headers = header_str;
	request->Server = "ballchasing.com";
	request->Page = path;
	request->Method = "POST";
	request->UserAgent = userAgent;
	request->Port = INTERNET_DEFAULT_HTTPS_PORT;
	request->ReqData = reqData;
	request->ReqDataSize = body_str.size();
	request->RespData = new char[4096];
	request->RespDataSize = 4096;
	request->RequestComplete = &RequestComplete;
	request->Flags = INTERNET_FLAG_SECURE;

	HttpRequestAsync(request);

	return true;
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