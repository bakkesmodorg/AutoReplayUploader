#include "AutoReplayUploaderPlugin.h"
#include "bakkesmod/wrappers/GameEvent/ReplayWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplayDirectorWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplaySoccarWrapper.h"
#include "utils/io.h"
#include <sstream>
#include <windows.h>
#include "isteamfriends.h"

BAKKESMOD_PLUGIN(AutoReplayUploaderPlugin, "Auto replay uploader plugin", "0.1", 0)

HTTPRequestHandle hdl;
string GetPlaylistName(int playlistId);

AutoReplayUploaderPlugin::AutoReplayUploaderPlugin()
{
	std::stringstream userAgentStream;
	userAgentStream << exports.className << "/" << exports.pluginVersion << " BakkesModAPI/" << BAKKESMOD_PLUGIN_API_VERSION;
	userAgent = userAgentStream.str();
}

std::string GenerateUrl(std::string baseUrl, std::map<std::string, std::string> getParams)
{
	std::stringstream urlStream;
	urlStream << baseUrl;
	if (!getParams.empty())
	{
		urlStream << "?";

		for (auto it = getParams.begin(); it != getParams.end(); it++)
		{
			if (it != getParams.begin())
				urlStream << "&";
			urlStream << (*it).first << "=" << (*it).second;
		}
	}
	return urlStream.str();
}

void AutoReplayUploaderPlugin::onLoad()
{
	cvarManager->log("onLoad.Start");
	HMODULE steamApi = GetModuleHandle("steam_api.dll");
	if (steamApi == NULL)
	{
		cvarManager->log("Steam API dll not loaded, note sure how this is possible!");
		return;
	}
	ISteamClient* steamClient = (ISteamClient*)((uintptr_t(__cdecl*)(void))GetProcAddress(steamApi, "SteamClient"))();

	if (steamClient == NULL)
	{
		cvarManager->log("Could not find Steam client, cancelling plugin load");
		return;
	}

	HSteamUser steamUser = (HSteamUser)((uintptr_t(__cdecl*)(void))GetProcAddress(steamApi, "SteamAPI_GetHSteamUser"))();

	if (steamUser == NULL)
	{
		cvarManager->log("Could not find Steam user, cancelling plugin load");
		return;
	}

	HSteamPipe steamPipe = (HSteamPipe)((uintptr_t(__cdecl*)(void))GetProcAddress(steamApi, "SteamAPI_GetHSteamPipe"))();
	
	if (steamPipe == NULL)
	{
		cvarManager->log("Could not find Steam pipe, cancelling plugin load");
		return;
	}

	ISteamHTTP* steamHTTPInstanceRet = (ISteamHTTP*)steamClient->GetISteamHTTP(steamUser, steamPipe, "STEAMHTTP_INTERFACE_VERSION002");
	
	if (steamHTTPInstanceRet == NULL)
	{
		cvarManager->log("Could not find Steam HTTP instance, cancelling plugin load");
		return;
	}

	steamUserName = steamClient->GetISteamFriends(steamUser, steamPipe, "SteamFriends017")->GetPersonaName();
	
	steamHTTPInstance = steamHTTPInstanceRet; 
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
	

	//Auth token response, stored in cvar so we can display it in the plugins tab. Should not be exposed to user!
	cvarManager->registerCvar("cl_autoreplayupload_ballchasing_testkeyresult", "Untested", "Auth token needed to upload replays to ballchasing.com", false, false, 0, false, 0, false);
	cvarManager->registerCvar("cl_autoreplayupload_ballchasing_visibility", "public", "Replay visibility when uploading to ballchasing.com", false, false, 0, false, 0, false).addOnValueChanged([this](std::string oldValue, CVarWrapper cvar)
	{
		//cvarManager->log(GenerateUrl(BALLCHASING_ENDPOINT_DEFAULT, { { "visibility", cvar.getStringValue() } }));
	});
	cvarManager->registerCvar("cl_autoreplayupload_ballchasing_authkey", "", "Auth token needed to upload replays to ballchasing.com").addOnValueChanged([this](std::string oldVal, CVarWrapper cvar)
	{
		//User changed authkey, reset testkeyresult
		cvarManager->getCvar("cl_autoreplayupload_ballchasing_testkeyresult").setValue("Untested");
	});
	cvarManager->registerNotifier("cl_autoreplayupload_ballchasing_testkey", std::bind(&AutoReplayUploaderPlugin::TestBallchasingAuth, this, std::placeholders::_1), 
		"Checks whether ballchasing authkey is valid", PERMISSION_ALL);
	//cvarManager->registerCvar("cl_autoreplayupload_calculated_endpoint", CALCULATED_ENDPOINT_DEFAULT, "URL to upload replay to when uploading to calculated.gg instance");

	cvarManager->registerCvar("cl_autoreplayupload_replaynametemplate", "", "Template for in game name of replay", true, true, 0, true, 0, true);
	cvarManager->registerCvar("cl_autoreplayupload_templatesequence", "0", "Current Template Sequence value to be used in replay name.", true, true, 0, false, 0, true).bindTo(templateSequence);

	/*
	Load notification assets
	*/
	//gameWrapper->LoadToastTexture("calculated_logo", "./bakkesmod/data/assets/calculated_logo.tga");
	//gameWrapper->LoadToastTexture("ballchasing_logo", "./bakkesmod/data/assets/ballchasing_logo.tga");
	cvarManager->registerCvar("cl_autoreplayupload_notifications", "1", "Show notifications on successful uploads", true, true, 0, true, 1).bindTo(showNotifications);
}

void AutoReplayUploaderPlugin::onUnload()
{
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void AutoReplayUploaderPlugin::SetReplayName(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay, std::string replayName)
{
	// Get Gamemode game was in
	auto playlist = server.GetPlaylist();
	auto mode = GetPlaylistName(playlist.GetPlaylistId());

	// Get current Sequence number
	auto seq = std::to_string(*templateSequence);
	*templateSequence = *templateSequence + 1; // increment to next sequence number

	// Get date string
	auto t = std::time(0);
	auto now = std::localtime(&t);
	auto date = std::to_string(now->tm_mon + 1) + "-" + std::to_string(now->tm_mday) + "-" + std::to_string(now->tm_year % 100) + " " + std::to_string(now->tm_hour) + ":" + std::to_string(now->tm_min);

	// Calculate Win/Loss string
	auto team = server.GetGameWinner();
	auto won = team.GetTeamIndex() == 0 ? soccarReplay.GetTeam0Score() > soccarReplay.GetTeam1Score() : soccarReplay.GetTeam1Score() > soccarReplay.GetTeam0Score();
	auto winloss = won ? string("W") : string("L");

	cvarManager->log("Username: " + steamUserName);
	cvarManager->log("Mode: " + mode);
	cvarManager->log("Sequence: " + seq);
	cvarManager->log("Date: " + date);
	cvarManager->log("WinLoss: " + winloss);

	replaceAll(replayName, "{PLAYER}", steamUserName);
	replaceAll(replayName, "{MODE}", mode);
	replaceAll(replayName, "{NUM}", seq);
	replaceAll(replayName, "{DATE}", date);
	replaceAll(replayName, "{WINLOSS}", winloss);

	cvarManager->log("ReplayName: " + replayName);
	soccarReplay.SetReplayName(replayName);
}
void AutoReplayUploaderPlugin::OnGameComplete(ServerWrapper caller, void * params, std::string eventName)
{
	cvarManager->log("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");

	if (!*uploadToCalculated && !*uploadToBallchasing)
	{
		return; //Not uploading replays
	}
	ReplayDirectorWrapper replayDirector = caller.GetReplayDirector();
	if (replayDirector.IsNull())
	{
		cvarManager->log("Could not upload replay, director is NULL!");
		//if(*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Error exporting replay! (1)", "default", 3.5f, ToastType_Error);
		return;
	}
	ReplaySoccarWrapper soccarReplay = replayDirector.GetReplay();
	if (soccarReplay.memory_address == NULL)
	{
		cvarManager->log("Could not upload replay, replay is NULL!");
		//if (*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Error exporting replay! (2)", "default", 3.5f, ToastType_Error);
		return;
	}
	std::string replayPath = "./bakkesmod/data/autoreplaysave.replay";// cvarManager->getCvar("cl_autoreplayupload_filepath").getStringValue(); //"./bakkesmod/data/autoreplaysave.replay";//
	if (file_exists(replayPath))
	{
		cvarManager->log("Removing existing file: " + replayPath);
		remove(replayPath.c_str());
	}

	auto replayNameTemplate = cvarManager->getCvar("cl_autoreplayupload_replaynametemplate").getStringValue();
	if (!replayNameTemplate.empty())
	{
		SetReplayName(caller, soccarReplay, replayNameTemplate);
	}

	cvarManager->log("Exporting replay to " + replayPath);
	soccarReplay.ExportReplay(replayPath); // %HOMEPATH%\Documents\My Games\Rocket League\TAGame\Demos
	cvarManager->log("Replay exported!");

	if (*uploadToCalculated) 
	{
		UploadReplayToEndpoint(replayPath, CALCULATED_ENDPOINT_DEFAULT, "replays", "", "calculated.gg");
	}
	if (*uploadToBallchasing)
	{
		std::string authKey = cvarManager->getCvar("cl_autoreplayupload_ballchasing_authkey").getStringValue();
		if (authKey.empty())
		{
			cvarManager->log("Cannot upload to ballchasing.com, no authkey set!");
			//if (*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Cannot upload to ballchasing.com, no authkey set!", "ballchasing_logo", 3.5f, ToastType_Error);
		}
		else 
		{
			std::string visibility = cvarManager->getCvar("cl_autoreplayupload_ballchasing_visibility").getStringValue();
			UploadReplayToEndpoint(replayPath, GenerateUrl(BALLCHASING_ENDPOINT_DEFAULT, { {"visibility", visibility} }), "file", authKey, "ballchasing.com");
		}
	}
	CheckFileUploadProgress(gameWrapper.get());
}

void AutoReplayUploaderPlugin::UploadReplayToEndpoint(std::string filename, std::string endpointUrl, std::string postName, std::string authKey, std::string endpointBaseUrl)
{
	cvarManager->log("UploadReplayToEndpoint.Start");
	std::vector<uint8> data = LoadReplay(filename);
	if (data.size() < 1)
	{
		cvarManager->log("Export failed! Aborting upload");
		//if (*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Error exporting replay! (3)", "default", 3.5f, ToastType_Error);
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
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "Content-Length", std::to_string(postData.size()).c_str());

	if (!steamHTTPInstance->SetHTTPRequestRawPostBody(hdl, contentType.str().c_str(), &postData[0], postData.size()))
	{
		cvarManager->log("Could not set post body, not uploading replay!");
		steamHTTPInstance->ReleaseHTTPRequest(hdl);
		return;
	}
	cvarManager->log("Full request body size: " + std::to_string(postData.size()));
	ReplayFileUploadData* uploadData = new ReplayFileUploadData();
	uploadData->requestHandle = hdl;
	uploadData->endpoint = endpointBaseUrl;
	uploadData->requester = this;
	steamHTTPInstance->SendHTTPRequest(uploadData->requestHandle, &uploadData->apiCall);
	uploadData->requestCompleteCallback.Set(uploadData->apiCall, uploadData, &FileUploadData::OnRequestComplete);

	fileUploadsInProgress.push_back(uploadData);

}

std::vector<uint8> AutoReplayUploaderPlugin::LoadReplay(std::string filename)
{
	cvarManager->log("AutoReplayUploaderPlugin.Start");
	std::ifstream replayFile(filename, std::ios::binary | std::ios::ate);
	std::streamsize replayFileSize = replayFile.tellg();
	if (replayFileSize < 100)
	{
		cvarManager->log("Replay size is too low, replay didn't export correctly?");
		return std::vector<uint8>();
	}
	replayFile.seekg(0, std::ios::beg);
	cvarManager->log("Replay size: " + std::to_string(replayFileSize));
	std::vector<uint8> data(replayFileSize, 0);
	data.reserve(replayFileSize);
	replayFile.read(reinterpret_cast<char*>(&data[0]), replayFileSize);
	cvarManager->log("Replay data size: " + std::to_string(data.size()));
	replayFile.close();
	return data;
}

void AutoReplayUploaderPlugin::CheckFileUploadProgress(GameWrapper * gw)
{
	cvarManager->log("CheckFileUploadProgress.Start");
	cvarManager->log("Running callback, files left to upload: " + std::to_string(fileUploadsInProgress.size()));
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
			

			cvarManager->log("Request successful: " + std::to_string((*it)->successful));
			cvarManager->log("Response code: " + std::to_string((*it)->statusCode));
			cvarManager->log("Response body size: " + std::to_string(body_size));
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

void AutoReplayUploaderPlugin::TestBallchasingAuth(std::vector<std::string> params)
{
	cvarManager->log("TestBallchasingAuth.Start");
	HTTPRequestHandle hdl = steamHTTPInstance->CreateHTTPRequest(k_EHTTPMethodGET, "https://ballchasing.com/api/");
	SteamAPICall_t* callHandle = NULL;
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "User-Agent", userAgent.c_str());

	std::string authKey = cvarManager->getCvar("cl_autoreplayupload_ballchasing_authkey").getStringValue();
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "Authorization", authKey.c_str());

	AuthKeyCheckUploadData* uploadData = new AuthKeyCheckUploadData(cvarManager);
	uploadData->requestHandle = hdl;
	uploadData->requester = this;
	steamHTTPInstance->SendHTTPRequest(uploadData->requestHandle, &uploadData->apiCall);
	uploadData->requestCompleteCallback.Set(uploadData->apiCall, uploadData, &FileUploadData::OnRequestComplete);
	
	fileUploadsInProgress.push_back(uploadData);
	CheckFileUploadProgress(gameWrapper.get());
}

void ReplayFileUploadData::OnRequestComplete(HTTPRequestCompleted_t * pCallback, bool failure)
{
	HTTPRequestData::OnRequestComplete(pCallback, failure);
	/*if (*((AutoReplayUploaderPlugin*)requester)->showNotifications) {
		std::string message = "Uploaded replay to " + endpoint + " successfully!";
		std::string logo_to_use = endpoint + "_logo";
		uint8_t toastType = ToastType_OK;
		if (!successful)
		{
			message = "Unable to upload replay to " + endpoint + ". (Network error)";
			toastType = ToastType_Error;
		}
		else if (!(statusCode >= 200 && statusCode < 300))
		{
			message = "Unable to upload replay to " + endpoint + ". (Server returned " + std::to_string(statusCode) + ")";
			toastType = ToastType_Error;
		}
		if (endpoint.find("."))
		{
			logo_to_use = endpoint.substr(0, endpoint.find(".") - 1) + "_logo";
		}
		requester->gameWrapper->Toast("Autoreplayuploader", message, logo_to_use, 3.5f, toastType);
	}*/
}

void AuthKeyCheckUploadData::OnRequestComplete(HTTPRequestCompleted_t * pCallback, bool failure)
{
	HTTPRequestData::OnRequestComplete(pCallback, failure);
	std::string result = "Invalid auth key!";
	//uint8_t toastType = ToastType_Warning;
	if (statusCode == 200)
	{
		result = "Auth key correct!";
		//toastType = ToastType_OK;
	}
	cvarManager->getCvar("cl_autoreplayupload_ballchasing_testkeyresult").setValue(result);
	//if (*((AutoReplayUploaderPlugin*)requester)->showNotifications) requester->gameWrapper->Toast("Autoreplayuploader", result, "ballchasing_logo", 3.5f, toastType);
}

string GetPlaylistName(int playlistId) {
	switch (playlistId) {
	case(1):
		return "Casual Duel";
		break;
	case(2):
		return "Casual Doubles";
		break;
	case(3):
		return "Casual Standard";
		break;
	case(4):
		return "Casual Chaos";
		break;
	case(6):
		return "Private";
		break;
	case(10):
		return "Ranked Duel";
		break;
	case(11):
		return "Ranked Doubles";
		break;
	case(12):
		return "Ranked Solo Standard";
		break;
	case(13):
		return "Ranked Standard";
		break;
	case(14):
		return "Mutator Mashup";
		break;
	case(22):
		return "Tournament";
		break;
	case(27):
		return "Ranked Hoops";
		break;
	case(28):
		return "Ranked Rumble";
		break;
	case(29):
		return "Ranked Dropshot";
		break;
	case(30):
		return "Ranked Snowday";
		break;
	default:
		return "";
		break;
	}
}