#include "AutoReplayUploaderPlugin.h"

#include <sstream>

#include "bakkesmod/wrappers/GameEvent/ReplayWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplayDirectorWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplaySoccarWrapper.h"

#include "utils/io.h"
#include "Utils.h"
#include "Ballchasing.h"
#include "Calculated.h"

#include <plog/Log.h>

BAKKESMOD_PLUGIN(AutoReplayUploaderPlugin, "Auto replay uploader plugin", "0.1", 0)

// Constant CVAR variable names
static const string CVAR_REPLAY_UPLOAD_PATH = "cl_autoreplayupload_filepath";
static const string CVAR_UPLOAD_TO_CALCULATED = "cl_autoreplayupload_calculated";
static const string CVAR_UPLOAD_TO_BALLCHASING = "cl_autoreplayupload_ballchasing";
static const string CVAR_BALLCHASING_AUTH_KEY = "cl_autoreplayupload_ballchasing_authkey";
static const string CVAR_BALLCHASING_AUTH_TEST_RESULT = "cl_autoreplayupload_ballchasing_testkeyresult";
static const string CVAR_BALLCHASING_REPLAY_VISIBILITY = "cl_autoreplayupload_ballchasing_visibility";
static const string CVAR_BALLCHASING_REPLAY_TESTKEY = "cl_autoreplayupload_ballchasing_testkey";
static const string CVAR_REPLAY_NAME_TEMPLATE = "cl_autoreplayupload_replaynametemplate";
static const string CVAR_REPLAY_SEQUENCE_NUM = "cl_autoreplayupload_replaysequence";
static const string CVAR_PLUGIN_SHOW_NOTIFICATIONS = "cl_autoreplayupload_notifications";

// TODO: uncomment or remove #ifdef's when new Bakkes mod API becomes available that has Toast notifications
// #define TOAST

string GetPlaylistName(int playlistId);

#pragma region AutoReplayUploaderPlugin Implementation

/**
* AutoReplayUploaderPlugin Constructor
*/
AutoReplayUploaderPlugin::AutoReplayUploaderPlugin()
{
	plog::init(plog::debug, "bakkesmod\\plugins\\ReplayUploader.log");
}

AutoReplayUploaderPlugin::~AutoReplayUploaderPlugin()
{
	LOG(plog::debug) << "AutoReplayUploader destructor invoked";
	delete logger;
}

/**
* OnLoad event called when the plugin is loaded by BakkesMod
*/
void AutoReplayUploaderPlugin::onLoad()
{
	LOG(plog::debug) << "AutoReplayUploader Plugin loaded";
	logger = new Logger(cvarManager);

	stringstream userAgentStream;
	userAgentStream << exports.className << "/" << exports.pluginVersion << " BakkesModAPI/" << BAKKESMOD_PLUGIN_API_VERSION;
	string userAgent = userAgentStream.str();

	// Setup upload handlers
	ballchasing = new Ballchasing(userAgent, "----BakkesModFileUpload90m8924r390j34f0", logger);
	calculated = new Calculated(userAgent, "----BakkesModFileUpload90m8924r390j34f0");

	// Register for Game ending event
	gameWrapper->HookEventWithCaller<ServerWrapper>(
		"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
		bind(
			&AutoReplayUploaderPlugin::OnGameComplete,
			this,
			placeholders::_1,
			placeholders::_2,
			placeholders::_3
		)
		);

	InitPluginVariables();

	// Register UI button press to call TestBallchasingAuth
	cvarManager->registerNotifier(
		CVAR_BALLCHASING_REPLAY_TESTKEY,
		bind(
			&AutoReplayUploaderPlugin::TestBallchasingAuth,
			this,
			placeholders::_1
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

/**
* OnUnload event called when the plugin is unloaded by BakkesMod
*/
void AutoReplayUploaderPlugin::onUnload()
{
	LOG(plog::debug) << "AutoReplayUploader Plugin unloaded";

	delete ballchasing;
	delete calculated;
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
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_KEY, "", "Auth token needed to upload replays to ballchasing.com").addOnValueChanged([this](string oldVal, CVarWrapper cvar)
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

/**
* OnGameComplete event called when on Function TAGame.GameEvent_Soccar_TA.EventMatchEnded event when an online game ends.
* Params:
*	caller - ServerWraper this event was called from
*	params - Event parameters
*	eventName - Event name
*/
void AutoReplayUploaderPlugin::OnGameComplete(ServerWrapper caller, void * params, string eventName)
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
		if (*showNotifications) gameWrapper->Toast("Autoreplayuploader", "Error exporting replay! (1)", "default", 3.5f, ToastType_Error);
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
	stringstream path;
	path << cvarManager->getCvar(CVAR_REPLAY_UPLOAD_PATH).getStringValue() << string("/") << "test" << ".replay";
	string replayPath = path.str();
	if (file_exists(replayPath))
	{
		cvarManager->log("Removing existing file: " + replayPath);
		remove(replayPath.c_str());
	}

	// Export Replay
	soccarReplay.ExportReplay(replayPath); // %HOMEPATH%\Documents\My Games\Rocket League\TAGame\Demos
	cvarManager->log("Exported replay to " + replayPath);

	// Upload replay
	//if (*uploadToCalculated)
	//{
	//	if (calculated->UploadReplay(replayPath) == true)
	//	{
	//		cvarManager->log("Uploaded replay to Calculated.gg");
	//	}
	//	else
	//	{
	//		cvarManager->log("Failed to upload replay to Calculated.gg");
	//	}
	//}
	if (*uploadToBallchasing)
	{
		string authKey = cvarManager->getCvar(CVAR_BALLCHASING_AUTH_KEY).getStringValue();
		string visibility = cvarManager->getCvar(CVAR_BALLCHASING_REPLAY_VISIBILITY).getStringValue();
		if (ballchasing->UploadReplay(replayPath, authKey, visibility) == true)
		{
			cvarManager->log("Uploaded Replay to Ballchasing.com");
		}
		else
		{
			cvarManager->log("Failed to upload replay to Ballchasing.com");
		}
	}
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
*		{WL} - W or L depending on if the player won or lost
*		{WINLOSS} - Win or Loss depending on if the player won or lost
*/
void AutoReplayUploaderPlugin::SetReplayName(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay, string replayName)
{
	// Get Gamemode game was in
	auto playlist = server.GetPlaylist();
	auto mode = GetPlaylistName(playlist.GetPlaylistId());

	// Get current Sequence number
	auto seq = to_string(*templateSequence);
	cvarManager->getCvar(CVAR_REPLAY_SEQUENCE_NUM).setValue(*templateSequence + 1); 
	cvarManager->executeCommand("writeconfig"); // since we change this variable ourselves we want to write the config when it changes so it persists across loads

	// Get date string
	auto t = time(0);
	auto now = localtime(&t);

	auto month = to_string(now->tm_mon + 1);
	month.insert(month.begin(), 2 - month.length(), '0');

	auto day = to_string(now->tm_mday);
	day.insert(day.begin(), 2 - day.length(), '0');

	auto year = to_string(now->tm_year + 1900);

	auto hour = to_string(now->tm_hour);
	hour.insert(hour.begin(), 2 - hour.length(), '0');

	auto min = to_string(now->tm_min);
	min.insert(min.begin(), 2 - min.length(), '0');

	// Calculate Win/Loss string
	auto team = server.GetGameWinner();
	auto won = team.GetTeamIndex() == 0 ? soccarReplay.GetTeam0Score() > soccarReplay.GetTeam1Score() : soccarReplay.GetTeam1Score() > soccarReplay.GetTeam0Score();
	auto winloss = won ? string("Win") : string("Loss");
	auto wl = won ? string("W") : string("L");

	cvarManager->log("Username: tyni");
	cvarManager->log("Mode: " + mode);
	cvarManager->log("Sequence: " + seq);
	cvarManager->log("Date: " + year + "-" + month + "-" + day + " " + hour + ":" + min);
	cvarManager->log("WinLoss: " + winloss);

	ReplaceAll(replayName, "{PLAYER}", "tyni");
	ReplaceAll(replayName, "{MODE}", mode);
	ReplaceAll(replayName, "{NUM}", seq);
	ReplaceAll(replayName, "{YEAR}", year);
	ReplaceAll(replayName, "{MONTH}", month);
	ReplaceAll(replayName, "{DAY}", day);
	ReplaceAll(replayName, "{HOUR}", hour);
	ReplaceAll(replayName, "{MIN}", min);
	ReplaceAll(replayName, "{WINLOSS}", winloss);
	ReplaceAll(replayName, "{WL}", wl);

	cvarManager->log("ReplayName: " + replayName);
	soccarReplay.SetReplayName(replayName);
}

/**
* Tests the authorization key for Ballchasing.com
*/
void AutoReplayUploaderPlugin::TestBallchasingAuth(std::vector<std::string> params)
{
	ballchasing->TestAuthKey(cvarManager->getCvar(CVAR_BALLCHASING_AUTH_KEY).getStringValue());
}

#pragma endregion

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

#pragma endregion