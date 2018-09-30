#include "AutoReplayUploaderPlugin.h"
#include "bakkesmod/wrappers/GameEvent/ReplayWrapper.h"
#include "utils/io.h"
#include <sstream>
#include <windows.h>

BAKKESMOD_PLUGIN(AutoReplayUploaderPlugin, "Auto replay uploader plugin", "0.1", 0)

HTTPRequestHandle hdl;

AutoReplayUploaderPlugin::AutoReplayUploaderPlugin()
{
	std::stringstream userAgentStream;
	userAgentStream << exports.className << "/" << exports.pluginVersion << " BakkesModAPI/" << BAKKESMOD_PLUGIN_API_VERSION;
	userAgent = userAgentStream.str();
}

void AutoReplayUploaderPlugin::onLoad()
{
	HMODULE steamApi = GetModuleHandle("steam_api.dll");
	if (steamApi == NULL)
	{
		cvarManager->log("Steam API dll not loaded, note sure how this is possible!");
		return;
	}
	steamHTTPInstance = (ISteamHTTP*)((uintptr_t(__cdecl*)(void))GetProcAddress(steamApi, "SteamHTTP"))();
	
	if (steamHTTPInstance == NULL)
	{
		cvarManager->log("Could not find Steam HTTP instance, cancelling plugin load");
		return;
	}

	SteamAPI_RunCallbacks_Function = (SteamAPI_RunCallbacks_typedef)(GetProcAddress(steamApi, "SteamAPI_RunCallbacks"));
	SteamAPI_RegisterCallResult_Function = (SteamAPI_RegisterCallResult_typedef)(GetProcAddress(steamApi, "SteamAPI_RegisterCallResult"));
	SteamAPI_UnregisterCallResult_Function = (SteamAPI_RegisterCallResult_typedef)(GetProcAddress(steamApi, "SteamAPI_UnregisterCallResult"));

	if (SteamAPI_RunCallbacks_Function == NULL || SteamAPI_RegisterCallResult_Function == NULL || SteamAPI_UnregisterCallResult_Function == NULL)
	{
		cvarManager->log("Could not find all functions in SteamAPI DLL!");
		return;
	}

	gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", 
		std::bind(&AutoReplayUploaderPlugin::OnGameComplete, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	//cvarManager->registerCvar("cl_autoreplayupload_filepath", "./bakkesmod/data/autoreplaysave.replay", "Path to save to be uploaded replay to.");
	cvarManager->registerCvar("cl_autoreplayupload_calculated", "0", "Upload to replays to calculated.gg automatically", true, true, 0, true, 1).bindTo(uploadToCalculated);
	cvarManager->registerCvar("cl_autoreplayupload_ballchasing", "0", "Upload to replays to ballchasing.com automatically", true, true, 0, true, 1).bindTo(uploadToBallchasing);

	cvarManager->registerCvar("cl_autoreplayupload_ballchasing_authkey", "", "Auth token needed to upload replays to ballchasing.com");
	//cvarManager->registerCvar("cl_autoreplayupload_calculated_endpoint", CALCULATED_ENDPOINT_DEFAULT, "URL to upload replay to when uploading to calculated.gg instance");

}

void AutoReplayUploaderPlugin::onUnload()
{
}

void AutoReplayUploaderPlugin::OnGameComplete(ServerWrapper caller, void * params, std::string eventName)
{
	if (!*uploadToCalculated && !*uploadToBallchasing)
	{
		return; //Not uploading replays
	}
	ReplayDirectorWrapper replayDirector = caller.GetReplayDirector();
	if (replayDirector.IsNull())
	{
		cvarManager->log("Could not upload replay, director is NULL!");
		return;
	}
	ReplaySoccarWrapper soccarReplay = replayDirector.GetReplay();
	if (soccarReplay.memory_address == NULL)
	{
		cvarManager->log("Could not upload replay, replay is NULL!");
		return;
	}
	std::string replayPath = "./bakkesmod/data/autoreplaysave.replay";// cvarManager->getCvar("cl_autoreplayupload_filepath").getStringValue(); //"./bakkesmod/data/autoreplaysave.replay";//
	if (file_exists(replayPath))
	{
		cvarManager->log("Removing existing file: " + replayPath);
		remove(replayPath.c_str());
	}
	cvarManager->log("Exporting replay to " + replayPath);
	soccarReplay.ExportReplay(replayPath);
	cvarManager->log("Replay exported!");
	if (*uploadToCalculated) 
	{
		UploadReplayToEndpoint(replayPath, CALCULATED_ENDPOINT_DEFAULT, "replays", "");
	}
	if (*uploadToBallchasing)
	{
		std::string authKey = cvarManager->getCvar("cl_autoreplayupload_ballchasing_authkey").getStringValue();
		if (authKey.empty())
		{
			//TODO: toast
			cvarManager->log("Cannot upload to ballchasing.com, no authkey set!");
		}
		else 
		{
			UploadReplayToEndpoint(replayPath, BALLCHASING_ENDPOINT_DEFAULT, "file", authKey);
		}
	}
	CheckFileUploadProgress(gameWrapper.get());
}

void AutoReplayUploaderPlugin::UploadReplayToEndpoint(std::string filename, std::string endpointUrl, std::string postName, std::string authKey)
{
	std::vector<uint8> data = LoadReplay(filename);
	if (data.size() < 1)
	{
		cvarManager->log("Export failed! Aborting upload");
		return;
	}
	cvarManager->log("Uploading replay to " + endpointUrl);
	HTTPRequestHandle hdl;
	hdl = steamHTTPInstance->CreateHTTPRequest(k_EHTTPMethodPOST, endpointUrl.c_str());
	SteamAPICall_t* callHandle = NULL;
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "User-Agent", userAgent.c_str());
	if (!authKey.empty())
	{
		steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "Authorization", authKey.c_str());
	}
	std::stringstream postBody;
	postBody << "--" << UPLOAD_BOUNDARY << "\r\n";
	postBody << "Content-Disposition: form-data; name=\"" << postName << "\"; filename=\"autosavedreplay.replay\"" << "\r\n";
	postBody << "Content-Type: multipart/form-data" << "\r\n";
	postBody << "\r\n";
	postBody << std::string(data.begin(), data.end());
	postBody << "\r\n";
	postBody << "--" << UPLOAD_BOUNDARY << "--" << "\r\n";

	auto postBodyString = postBody.str();
	postData = std::vector<uint8>(postBodyString.begin(), postBodyString.end());

	std::stringstream contentType;
	contentType << "multipart/form-data;boundary=" << UPLOAD_BOUNDARY << "";
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "Content-Length", to_string(postData.size()).c_str());

	if (!steamHTTPInstance->SetHTTPRequestRawPostBody(hdl, contentType.str().c_str(), &postData[0], postData.size()))
	{
		cvarManager->log("Could not set post body, not uploading replay!");
		steamHTTPInstance->ReleaseHTTPRequest(hdl);
		return;
	}
	cvarManager->log("Full request body size: " + to_string(postData.size()));
	FileUploadData* uploadData = new FileUploadData();
	uploadData->requestHandle = hdl;
	steamHTTPInstance->SendHTTPRequest(uploadData->requestHandle, &uploadData->apiCall);
	uploadData->requestCompleteCallback.Set(uploadData->apiCall, uploadData, &FileUploadData::OnRequestComplete);

	fileUploadsInProgress.push_back(uploadData);

}

std::vector<uint8> AutoReplayUploaderPlugin::LoadReplay(std::string filename)
{
	std::ifstream replayFile(filename, std::ios::binary | std::ios::ate);
	std::streamsize replayFileSize = replayFile.tellg();
	if (replayFileSize < 100)
	{
		cvarManager->log("Replay size is too low, replay didn't export correctly?");
		return std::vector<uint8>();
	}
	replayFile.seekg(0, std::ios::beg);
	cvarManager->log("Replay size: " + to_string(replayFileSize));
	std::vector<uint8> data(replayFileSize, 0);
	data.reserve(replayFileSize);
	replayFile.read(reinterpret_cast<char*>(&data[0]), replayFileSize);
	cvarManager->log("Replay data size: " + to_string(data.size()));
	replayFile.close();
	return data;
}

void AutoReplayUploaderPlugin::CheckFileUploadProgress(GameWrapper * gw)
{
	cvarManager->log("Running callback, files left to upload: " + to_string(fileUploadsInProgress.size()));
	SteamAPI_RunCallbacks_Function();
	cvarManager->log("Executed Steam callbacks");
	for (auto it = fileUploadsInProgress.begin(); it != fileUploadsInProgress.end();)
	{
		if ((*it)->canBeDeleted)
		{
			uint8 buf[4096];
			buf[0] = buf[4095] = '\0';
			uint32 body_size = 0;
			steamHTTPInstance->GetHTTPResponseBodySize((*it)->requestHandle, &body_size);
			//Let buffer max be 4096 (save last byte for nullbyte)
			body_size = min(4095, body_size);
			steamHTTPInstance->GetHTTPResponseBodyData((*it)->requestHandle, buf, body_size);
			

			cvarManager->log("Request successful: " + to_string((*it)->successful));
			cvarManager->log("Response code: " + to_string((*it)->statusCode));
			cvarManager->log("Response body size: " + to_string(body_size));
			cvarManager->log("Response body: " + std::string(buf, buf + body_size));
			steamHTTPInstance->ReleaseHTTPRequest((*it)->requestHandle);
			delete (*it);
			cvarManager->log("Erased request");
			it = fileUploadsInProgress.erase(it);
		}
		else
		{
			it++;
		}
	}
	if (!fileUploadsInProgress.empty())
	{
		gw->SetTimeout(std::bind(&AutoReplayUploaderPlugin::CheckFileUploadProgress, this, std::placeholders::_1), .5f);
	}
}

