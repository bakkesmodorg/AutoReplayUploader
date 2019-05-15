#include "AutoReplayUploaderPlugin.h"
#include "bakkesmod/wrappers/GameEvent/ReplayWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplayDirectorWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplaySoccarWrapper.h"
#include "utils/io.h"
#include <sstream>
#include <windows.h>
#include "isteamfriends.h"

BAKKESMOD_PLUGIN(AutoReplayUploaderPlugin, "Auto replay uploader plugin", "0.1", 0)

// Constant CVAR variable names
static const string CVAR_REPLAY_UPLOAD_PATH = "cl_autoreplayupload_filepath";
static const string CVAR_UPLOAD_TO_CALCULATED = "cl_autoreplayupload_calculated";
static const string CVAR_UPLOAD_TO_BALLCHASING = "cl_autoreplayupload_ballchasing";
static const string CVAR_BALLCHASING_AUTH_KEY = "cl_autoreplayupload_ballchasing_authkey";
static const string CVAR_BALLCHASING_AUTH_TEST_RESULT = "cl_autoreplayupload_ballchasing_testkeyresult";
static const string CVAR_BALLCHASING_REPLAY_VISIBILITY = "cl_autoreplayupload_ballchasing_visibility";
static const string CVAR_REPLAY_NAME_TEMPLATE = "cl_autoreplayupload_replaynametemplate";
static const string CVAR_REPLAY_SEQUENCE_NUM = "cl_autoreplayupload_replaysequence";
static const string CVAR_PLUGIN_SHOW_NOTIFICATIONS = "cl_autoreplayupload_notifications";

// TODO: uncomment or remove #ifdef's when new Bakkes mod API becomes available that has Toast notifications
// #define TOAST

#pragma region PreDeclarations
HTTPRequestHandle hdl;
string GetPlaylistName(int playlistId);
std::string GenerateUrl(std::string baseUrl, std::map<std::string, std::string> getParams);
void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
#pragma endregion

#pragma region AutoReplayUploaderPlugin Implementation

/**
* AutoReplayUploaderPlugin Constructor
*/
AutoReplayUploaderPlugin::AutoReplayUploaderPlugin()
{
	std::stringstream userAgentStream;
	userAgentStream << exports.className << "/" << exports.pluginVersion << " BakkesModAPI/" << BAKKESMOD_PLUGIN_API_VERSION;
	userAgent = userAgentStream.str();
}

/**
* OnLoad event called when the plugin is loaded by BakkesMod
*/
void AutoReplayUploaderPlugin::onLoad()
{
	InitSteamClient();

	// Register for Game ending event
	gameWrapper->HookEventWithCaller<ServerWrapper>(
		"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", 
		std::bind(
			&AutoReplayUploaderPlugin::OnGameComplete, 
			this, 
			std::placeholders::_1, 
			std::placeholders::_2, 
			std::placeholders::_3
		)
	);

	InitPluginVariables();

	// Register UI button press to call TestBallchasingAuth
	cvarManager->registerNotifier(
		"cl_autoreplayupload_ballchasing_testkey", 
		std::bind(
			&AutoReplayUploaderPlugin::TestBallchasingAuth, 
			this, 
			std::placeholders::_1
		),
		"Checks whether ballchasing authkey is valid", 
		PERMISSION_ALL
	);

	// Initialize notification plugin assets
#ifdef TOAST
	gameWrapper->LoadToastTexture("calculated_logo", "./bakkesmod/data/assets/calculated_logo.tga");
	gameWrapper->LoadToastTexture("ballchasing_logo", "./bakkesmod/data/assets/ballchasing_logo.tga");
#endif
}

void AutoReplayUploaderPlugin::InitPluginVariables()
{
	// Path to export replays to
	cvarManager->registerCvar(CVAR_REPLAY_UPLOAD_PATH, "./bakkesmod/data", "Path to save the exported replays to.");

	// What endpoints should we upload to?
	cvarManager->registerCvar(CVAR_UPLOAD_TO_CALCULATED, "0", "Upload to replays to calculated.gg automatically", true, true, 0, true, 1).bindTo(uploadToCalculated);
	cvarManager->registerCvar(CVAR_UPLOAD_TO_BALLCHASING, "0", "Upload to replays to ballchasing.com automatically", true, true, 0, true, 1).bindTo(uploadToBallchasing);

	// Ball Chasing variables
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT, "Untested", "Auth token needed to upload replays to ballchasing.com", false, false, 0, false, 0, false);
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_KEY, "", "Auth token needed to upload replays to ballchasing.com").addOnValueChanged([this](std::string oldVal, CVarWrapper cvar)
	{   // Auth token response, stored in cvar so we can display it in the plugins tab. Should not be exposed to user!
		// User changed authkey, reset testkeyresult
		cvarManager->getCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT).setValue("Untested");
	});
	cvarManager->registerCvar(CVAR_BALLCHASING_REPLAY_VISIBILITY, "public", "Replay visibility when uploading to ballchasing.com", false, false, 0, false, 0, false);

	// Replay Name template variables
	cvarManager->registerCvar(CVAR_REPLAY_NAME_TEMPLATE, "", "Template for in game name of replay", true, true, 0, true, 0, true);
	cvarManager->registerCvar(CVAR_REPLAY_SEQUENCE_NUM, "0", "Current Reqlay Sequence number to be used in replay name.", true, true, 0, false, 0, true).bindTo(templateSequence);

	// Notification variables
	cvarManager->registerCvar(CVAR_PLUGIN_SHOW_NOTIFICATIONS, "1", "Show notifications on successful uploads", true, true, 0, true, 1).bindTo(showNotifications);
}

void AutoReplayUploaderPlugin::InitSteamClient()
{
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

	// Finally setup steam http client instance we will use to upload replays
	steamHTTPInstance = (ISteamHTTP*)steamClient->GetISteamHTTP(steamUser, steamPipe, "STEAMHTTP_INTERFACE_VERSION002");
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

	steamUserName = steamClient->GetISteamFriends(steamUser, steamPipe, "SteamFriends017")->GetPersonaName();
}

/**
* OnUnload event called when the plugin is unloaded by BakkesMod
*/
void AutoReplayUploaderPlugin::onUnload()
{
}

/**
* OnGameComplete event called when on Function TAGame.GameEvent_Soccar_TA.EventMatchEnded event when an online game ends.
* Params:
*	caller - ServerWraper this event was called from
*	params - Event parameters
*	eventName - Event name
*/
void AutoReplayUploaderPlugin::OnGameComplete(ServerWrapper caller, void * params, std::string eventName)
{
	if (!*uploadToCalculated && !*uploadToBallchasing) // Bail if we aren't uploading replays
	{
		return; //Not uploading replays
	}

	// Get ReplayDirector
	ReplayDirectorWrapper replayDirector = caller.GetReplayDirector();
	if (replayDirector.IsNull())
	{
		cvarManager->log("Could not upload replay, director is NULL!");
#ifdef TOAST
		if(*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Error exporting replay! (1)", "default", 3.5f, ToastType_Error);
#endif
		return;
	}

	// Get Replay wrapper
	ReplaySoccarWrapper soccarReplay = replayDirector.GetReplay();
	if (soccarReplay.memory_address == NULL)
	{
		cvarManager->log("Could not upload replay, replay is NULL!");
#ifdef TOAST
		if (*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Error exporting replay! (2)", "default", 3.5f, ToastType_Error);
#endif
		return;
	}

	// If we have a template for the replay name then set the replay name based off that template
	auto replayNameTemplate = cvarManager->getCvar(CVAR_REPLAY_NAME_TEMPLATE).getStringValue();
	if (!replayNameTemplate.empty())
	{
		SetReplayName(caller, soccarReplay, replayNameTemplate);
	}

	// Determine the replay path and if it already exists remove it
	std::string replayPath = "./bakkesmod/data/autoreplaysave.replay";// cvarManager->getCvar(CVAR_REPLAY_UPLOAD_PATH).getStringValue();
	if (file_exists(replayPath))
	{
		cvarManager->log("Removing existing file: " + replayPath);
		remove(replayPath.c_str());
	}
	
	// Export Replay
	soccarReplay.ExportReplay(replayPath); // %HOMEPATH%\Documents\My Games\Rocket League\TAGame\Demos
	cvarManager->log("Exported replay to " + replayPath);

	// Upload replay
	if (*uploadToCalculated) 
	{
		UploadReplayToEndpoint(replayPath, CALCULATED);
	}
	if (*uploadToBallchasing)
	{
		UploadReplayToEndpoint(replayPath, BALLCHASING);
	}
	CheckFileUploadProgress(gameWrapper.get());
}

/**
* SetReplayName - Called to set the name of the replay in the replay file.
* Params:
*	server - ServerWrapper
*	soccarReplay - Replay to set name of
*	replayName - A templatized string that accepts the following tokens for replacement.
*		{PLAYER} - Name of current steam user
*		{MODE} - Game mode of replay (Private, Ranked Standard, etc...)
*		{NUM} - Current sequence number to allow for uniqueness
*		{YEAR} - Year since 1900 % 100, eg. 2019 returns 19
*		{MONTH} - Month 1-12
*		{DAY} - Day of the month 1-31
*		{HOUR} - Hour of the day 0-23
*		{MIN} - Min of the hour 0-59
*		{WINLOSS} - W or L depending on if the player won or lost
*/
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

	// Calculate Win/Loss string
	auto team = server.GetGameWinner();
	auto won = team.GetTeamIndex() == 0 ? soccarReplay.GetTeam0Score() > soccarReplay.GetTeam1Score() : soccarReplay.GetTeam1Score() > soccarReplay.GetTeam0Score();
	auto winloss = won ? string("W") : string("L");

	cvarManager->log("Username: " + steamUserName);
	cvarManager->log("Mode: " + mode);
	cvarManager->log("Sequence: " + seq);
	cvarManager->log("Date: " + std::to_string(now->tm_year % 100) +"-" + std::to_string(now->tm_mday) + "-" + std::to_string(now->tm_year % 100) + " " + std::to_string(now->tm_hour) + ":" + std::to_string(now->tm_min));
	cvarManager->log("WinLoss: " + winloss);

	ReplaceAll(replayName, "{PLAYER}", steamUserName);
	ReplaceAll(replayName, "{MODE}", mode);
	ReplaceAll(replayName, "{NUM}", seq);
	ReplaceAll(replayName, "{YEAR}", std::to_string(now->tm_year % 100));
	ReplaceAll(replayName, "{MONTH}", std::to_string(now->tm_mon + 1));
	ReplaceAll(replayName, "{DAY}", std::to_string(now->tm_mday));
	ReplaceAll(replayName, "{HOUR}", std::to_string(now->tm_hour));
	ReplaceAll(replayName, "{MIN}", std::to_string(now->tm_min));
	ReplaceAll(replayName, "{WINLOSS}", winloss);

	cvarManager->log("ReplayName: " + replayName);
	soccarReplay.SetReplayName(replayName);
}

/**
* UploadRelayToEndpoint - Uploads the filename to the endpoint
* Params:
*	filename - Path to replay file
*	endpoint - Endpoint to upload to
*/
void AutoReplayUploaderPlugin::UploadReplayToEndpoint(std::string filename, UploadEndpoints endpoint)
{
	// Params for the different endpoints
	std::string endpointUrl;
	std::string postName;
	std::string authKey;
	std::string endpointBaseUrl;

	// Setup params based off of what endpoints we are going to
	switch (endpoint)
	{
	case BALLCHASING:
		endpointUrl = GenerateUrl(BALLCHASING_ENDPOINT_DEFAULT, { {"visibility", cvarManager->getCvar(CVAR_BALLCHASING_REPLAY_VISIBILITY).getStringValue()} });
		postName = "file";
		endpointBaseUrl = "ballchasing.com";
		authKey = cvarManager->getCvar(CVAR_BALLCHASING_AUTH_KEY).getStringValue();
		if (authKey.empty())
		{
			cvarManager->log("Cannot upload to ballchasing.com, no authkey set!");
			//if (*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Cannot upload to ballchasing.com, no authkey set!", "ballchasing_logo", 3.5f, ToastType_Error);
		}
		break;
	case CALCULATED:
		endpointUrl = CALCULATED_ENDPOINT_DEFAULT;
		postName = "replays";
		authKey = "";
		endpointBaseUrl = "calculated.gg";
		break;
	}

	// Read the data bytes from the replay file
	std::vector<uint8> data = GetReplayBytes(filename);
	if (data.size() < 1)
	{
		cvarManager->log("Export failed! Aborting upload");
#ifdef TOAST
		if (*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Error exporting replay! (3)", "default", 3.5f, ToastType_Error);
#endif
		return;
	}
	
	// Initialize steam http client
	HTTPRequestHandle hdl;
	hdl = steamHTTPInstance->CreateHTTPRequest(k_EHTTPMethodPOST, endpointUrl.c_str());
	SteamAPICall_t* callHandle = NULL;

	// Construct body
	std::stringstream postBody;
	postBody << "--" << UPLOAD_BOUNDARY << "\r\n";
	postBody << "Content-Disposition: form-data; name=\"" << postName << "\"; filename=\"autosavedreplay.replay\"" << "\r\n";
	postBody << "Content-Type: multipart/form-data" << "\r\n";
	postBody << "\r\n";
	postBody << std::string(data.begin(), data.end());
	postBody << "\r\n";
	postBody << "--" << UPLOAD_BOUNDARY << "--" << "\r\n";
	auto postBodyString = postBody.str();
	postData = std::vector<uint8>(postBodyString.begin(), postBodyString.end()); // postData is a member variable does it need to be?

	// Setup HTTP Headers
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "Content-Length", std::to_string(postData.size()).c_str());
	steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "User-Agent", userAgent.c_str());
	if (endpoint == BALLCHASING)
	{
		steamHTTPInstance->SetHTTPRequestHeaderValue(hdl, "Authorization", authKey.c_str());
	}

	// Set Content-Type
	std::stringstream contentType;
	contentType << "multipart/form-data;boundary=" << UPLOAD_BOUNDARY << "";

	// Set HTTP Body
	if (!steamHTTPInstance->SetHTTPRequestRawPostBody(hdl, contentType.str().c_str(), &postData[0], postData.size()))
	{
		cvarManager->log("Could not set post body, not uploading replay!");
		steamHTTPInstance->ReleaseHTTPRequest(hdl);
		return;
	}

	cvarManager->log("Uploading replay to " + endpointUrl);
	cvarManager->log("Full request body size: " + std::to_string(postData.size()));

	// Construct upload data structure
	ReplayFileUploadData* uploadData = new ReplayFileUploadData();
	uploadData->requestHandle = hdl;
	uploadData->endpoint = endpointBaseUrl;
	uploadData->requester = this;

	// Send HTTP request
	steamHTTPInstance->SendHTTPRequest(uploadData->requestHandle, &uploadData->apiCall);

	// Register callback for when HTTP call completes
	uploadData->requestCompleteCallback.Set(uploadData->apiCall, uploadData, &FileUploadData::OnRequestComplete);

	// Add upload data to member vector to keep track of the number of uploads we have
	fileUploadsInProgress.push_back(uploadData);
}

/**
* GetReplayBytes - Loads the file specified in filename and returns the bytes
*/
std::vector<uint8> AutoReplayUploaderPlugin::GetReplayBytes(std::string filename)
{
	// Open file
	std::ifstream replayFile(filename, std::ios::binary | std::ios::ate);

	// Get and validate replay file size
	std::streamsize replayFileSize = replayFile.tellg();
	if (replayFileSize < 100) // Unless you start and finish a game in under 5 seconds the replay file will be larger than this
	{
		cvarManager->log("Replay size is too low, replay didn't export correctly?");
		return std::vector<uint8>();
	}
	cvarManager->log("Replay size: " + std::to_string(replayFileSize));
	
	// Initialize byte vector to size of replay
	std::vector<uint8> data(replayFileSize, 0);
	data.reserve(replayFileSize);

	// Read replay file from the beginning
	replayFile.seekg(0, std::ios::beg);
	replayFile.read(reinterpret_cast<char*>(&data[0]), replayFileSize);
	replayFile.close();

	cvarManager->log("Replay data size: " + std::to_string(data.size()));
	return data;
}

/**
* CheckFileUploadProgress - Checks the upload progress of the file.
*/
void AutoReplayUploaderPlugin::CheckFileUploadProgress(GameWrapper * gw)
{
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

/**
* Tests the authorization key for Ballchasing.com
*/
void AutoReplayUploaderPlugin::TestBallchasingAuth(std::vector<std::string> params)
{
	HTTPRequestHandle hdl = steamHTTPInstance->CreateHTTPRequest(k_EHTTPMethodGET, "https://ballchasing.com/api/");
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
	CheckFileUploadProgress(gameWrapper.get());
}

#pragma endregion

void AuthKeyCheckUploadData::OnRequestComplete(HTTPRequestCompleted_t * pCallback, bool failure)
{
	HTTPRequestData::OnRequestComplete(pCallback, failure);

	// Set variable so user can see in UI that the status of the auth key
	std::string result = statusCode == 200 ? "Auth key correct!" : "Invalid auth key!";
	cvarManager->getCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT).setValue(result);

#ifdef TOAST
	uint8_t toastType = statusCode == 200 ? ToastType_OK : ToastType_Warning;
	if (*((AutoReplayUploaderPlugin*)requester)->showNotifications) requester->gameWrapper->Toast("Autoreplayuploader", result, "ballchasing_logo", 3.5f, toastType);
#endif
}

void ReplayFileUploadData::OnRequestComplete(HTTPRequestCompleted_t * pCallback, bool failure)
{
	HTTPRequestData::OnRequestComplete(pCallback, failure);
#ifdef TOAST
	if (*((AutoReplayUploaderPlugin*)requester)->showNotifications) {
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
	}
#endif
}

#pragma region Utility Functions

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

void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

#pragma endregion
