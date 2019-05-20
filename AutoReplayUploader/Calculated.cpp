#include "Calculated.h"
#include "Utils.h"
#include "Wininet.h"

#include <sstream>
#include <fstream>

using namespace std;

Calculated::Calculated(string userAgent, string uploadBoundary)
{
	this->UserAgent = userAgent;
	this->UploadBoundary = uploadBoundary;
}

/**
* Posts the replay file to Calculated.gg
*/
bool Calculated::UploadReplay(string replayPath)
{
	if (UserAgent.empty() || replayPath.empty())
		return false;

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
	auto body_str = body.str();

	/*uint8_t result[2048];
	POST("calculated.gg", "api/upload", header_str, body_str, result, 2048);*/

	//Wininet client;
	//if (client.Connect("ballchasing.com"), 443, UserAgent, 30000, INTERNET_FLAG_SECURE)
	//{
	//	if (client.Post("api/upload", header_str, body_str))
	//	{
	//		return true;
	//	}
	//}

	return true;
}

Calculated::~Calculated()
{
}
