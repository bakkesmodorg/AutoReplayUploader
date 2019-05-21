#include "Ballchasing.h"
#include "Utils.h"

#include <sstream>
#include <fstream>

#include <plog/Log.h>

#include "Wininet.h"


Ballchasing::Ballchasing(string userAgent, string uploadBoundary, Logger* logger)
{
	this->userAgent = userAgent;
	this->uploadBoundary = uploadBoundary;
	this->logger = logger;
}

void RequestComplete(HttpRequestObject* ctx)
{
	LOG(plog::debug) << "RequestComplete";

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
	logger->Log("ReplayPath: " + replayPath);
	logger->Log("AuthKey: " + authKey);
	logger->Log("Visibility: " + visibility);

	if (userAgent.empty() || authKey.empty() || visibility.empty() || replayPath.empty())
	{
		logger->Log("Ballchasing::UploadReplay Parameters were empty.");
		return false;
	}

	logger->Log("Reading file bytes: " + replayPath);
	// Get Replay file bytes to upload
	auto bytes = GetFileBytes(replayPath);

	// Construct headers
	stringstream headers;
	headers << "Authorization: " << authKey << "\r\n";
	headers << "Content-Type: multipart/form-data;boundary=" << uploadBoundary;
	auto header_str = headers.str();

	logger->Log("Headers: " + header_str);

	// Construct body
	stringstream body;
	body << "--" << uploadBoundary << "\r\n";
	body << "Content-Disposition: form-data; name=\"file\"; filename=\"autosavedreplay.replay\"" << "\r\n";
	body << "Content-Type: application/form-data" << "\r\n";
	body << "\r\n";
	body << string(bytes.begin(), bytes.end());
	body << "\r\n";
	body << "--" << uploadBoundary << "--" << "\r\n";

	vector<uint8_t> buffer;
	const string& str = body.str();
	buffer.insert(buffer.end(), str.begin(), str.end());

	char *reqData = new char[buffer.size() + 1];
	for (int i = 0; i < buffer.size(); i++)
		reqData[i] = buffer[i];
	reqData[buffer.size()] = '\0';

	cout << reqData;

	string path = AppendGetParams("api/upload", { {"visibility", visibility} });

	HttpRequestObject* ctx = new HttpRequestObject();
	ctx->RequestId = 1;
	ctx->Requester = this;
	ctx->Headers = header_str;
	ctx->Server = "ballchasing.com";
	ctx->Page = path;
	ctx->Method = "POST";
	ctx->UserAgent = userAgent;
	ctx->Port = INTERNET_DEFAULT_HTTPS_PORT;
	ctx->ReqData = reqData;
	ctx->ReqDataSize = buffer.size();
	ctx->RespData = new char[4096];
	ctx->RespDataSize = 4096;
	ctx->RequestComplete = &RequestComplete;
	ctx->Flags = INTERNET_FLAG_SECURE;

	logger->Log("ReqDataSize: " + to_string(ctx->ReqDataSize));
	logger->Log("Path: " + path);

	HttpRequestAsync(ctx);

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