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

void ReplayUploadCompleted(void* requester, DWORD status, char* result, size_t result_length)
{
	auto ballchasing = (Ballchasing*)requester;
	ballchasing->UploadCompleted(status);
}

void Ballchasing::UploadCompleted(DWORD status)
{
	logger->Log("UploadCompleted with status: " + to_string(status));
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

	string path = AppendGetParams("api/upload", { {"visibility", visibility} });

	Wininet client(logger);
	if (client.Connect("ballchasing.com"), 443, userAgent, 30000, INTERNET_FLAG_SECURE)
	{
		client.Post(path, header_str, body_str, uploadReplayResult, 4096, ReplayUploadCompleted, this);
	}

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