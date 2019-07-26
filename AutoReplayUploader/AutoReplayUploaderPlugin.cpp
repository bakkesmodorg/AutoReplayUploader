#include "AutoReplayUploaderPlugin.h"

#include <sstream>
#include <utils/io.h>

#include "bakkesmod/wrappers/GameEvent/ReplayWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplayDirectorWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplaySoccarWrapper.h"

#include "Utils.h"
#include "Ballchasing.h"
#include "Calculated.h"
#include "Match.h"
#include "Player.h"
#include "Replay.h"

using namespace std;

BAKKESMOD_PLUGIN(AutoReplayUploaderPlugin, "Auto replay uploader plugin", "0.1", 0);

// Constant CVAR variable names
#define CVAR_REPLAY_EXPORT_PATH "cl_autoreplayupload_filepath"
#define CVAR_REPLAY_EXPORT "cl_autoreplayupload_save"
#define CVAR_UPLOAD_TO_CALCULATED "cl_autoreplayupload_calculated"
#define CVAR_UPLOAD_TO_BALLCHASING "cl_autoreplayupload_ballchasing"
#define CVAR_REPLAY_NAME_TEMPLATE "cl_autoreplayupload_replaynametemplate"
#define CVAR_REPLAY_SEQUENCE_NUM "cl_autoreplayupload_replaysequence"
#define CVAR_PLUGIN_SHOW_NOTIFICATIONS "cl_autoreplayupload_notifications"
#define CVAR_BALLCHASING_AUTH_KEY "cl_autoreplayupload_ballchasing_authkey"
#define CVAR_BALLCHASING_AUTH_TEST_RESULT "cl_autoreplayupload_ballchasing_testkeyresult"
#define CVAR_BALLCHASING_REPLAY_VISIBILITY "cl_autoreplayupload_ballchasing_visibility"
#define CVAR_CALCULATED_REPLAY_VISIBILITY "cl_autoreplayupload_calculated_visibility"

string GetPlaylistName(int playlistId);

void Log(void* object, string message)
{
	auto plugin = (AutoReplayUploaderPlugin*)object;
	plugin->cvarManager->log(message);
}

void UploadComplete(AutoReplayUploaderPlugin* plugin, bool result, string endpoint)
{
#ifdef TOAST
	if (*(plugin->showNotifications)) {
		std::string message = "Uploaded replay to " + endpoint + " successfully!";
		std::string logo_to_use = endpoint + "_logo";
		uint8_t toastType = ToastType_OK;
		if (!result)
		{
			message = "Unable to upload replay to " + endpoint + ".";
			toastType = ToastType_Error;
		}
		if (endpoint.find("."))
		{
			logo_to_use = endpoint.substr(0, endpoint.find(".") - 1) + "_logo";
		}
		plugin->gameWrapper->Toast("Autoreplayuploader", message, logo_to_use, 3.5f, toastType);
	}
#endif
}

void CalculatedUploadComplete(void* object, bool result)
{
	UploadComplete((AutoReplayUploaderPlugin*)object, result, "calculated");
}

void BallchasingUploadComplete(void* object, bool result)
{
	UploadComplete((AutoReplayUploaderPlugin*)object, result, "ballchasing");
}

void BallchasingAuthTestComplete(void* object, bool result)
{
	auto plugin = (AutoReplayUploaderPlugin*)object;
	string msg = result ? "Auth key correct!" : "Invalid auth key!";
	plugin->cvarManager->getCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT).setValue(msg);
}

#pragma region AutoReplayUploaderPlugin Implementation

/**
* OnLoad event called when the plugin is loaded by BakkesMod
*/
void AutoReplayUploaderPlugin::onLoad()
{
	stringstream userAgentStream;
	userAgentStream << exports.className << "/" << exports.pluginVersion << " BakkesModAPI/" << BAKKESMOD_PLUGIN_API_VERSION;
	string userAgent = userAgentStream.str();

	// Setup upload handlers
	ballchasing = new Ballchasing(userAgent, &Log, &BallchasingUploadComplete, &BallchasingAuthTestComplete, this);
	calculated = new Calculated(userAgent, &Log, &CalculatedUploadComplete, this);

	InitializeVariables();

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
	delete ballchasing;
	delete calculated;
}

void AutoReplayUploaderPlugin::InitializeVariables()
{
	// Calculated variables
	cvarManager->registerCvar(CVAR_UPLOAD_TO_CALCULATED, "0", "Upload to replays to calculated.gg automatically", true, true, 0, true, 1).bindTo(uploadToCalculated);		
	cvarManager->registerCvar(CVAR_CALCULATED_REPLAY_VISIBILITY, "DEFAULT", "Replay visibility when uploading to calculated.gg", false, false, 0, false, 0, false).bindTo(calculated->visibility);

	// Ball Chasing variables	
	cvarManager->registerCvar(CVAR_UPLOAD_TO_BALLCHASING, "0", "Upload to replays to ballchasing.com automatically", true, true, 0, true, 1).bindTo(uploadToBallchasing);
	cvarManager->registerCvar(CVAR_BALLCHASING_REPLAY_VISIBILITY, "public", "Replay visibility when uploading to ballchasing.com", false, false, 0, false, 0, true).bindTo(ballchasing->visibility);
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT, "Untested", "Auth token needed to upload replays to ballchasing.com", false, false, 0, false, 0, false);
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_KEY, "", "Auth token needed to upload replays to ballchasing.com").bindTo(ballchasing->authKey);
	cvarManager->getCvar(CVAR_BALLCHASING_AUTH_KEY).addOnValueChanged([this](string oldVal, CVarWrapper cvar)
	{   
		if (ballchasing->authKey->size() > 0 &&         // We don't test the auth key if the size of the auth key is empty
			ballchasing->authKey->compare(oldVal) != 0) // We don't test unless the value has changed
		{
			// value changed so test auth key
			ballchasing->TestAuthKey();
		}
	});

	// Replay Name template variables
	cvarManager->registerCvar(CVAR_REPLAY_SEQUENCE_NUM, "0", "Current Reqlay Sequence number to be used in replay name", true, true, 0, false, 0, true).bindTo(templateSequence);
	cvarManager->registerCvar(CVAR_REPLAY_NAME_TEMPLATE, DEFAULT_REPLAY_NAME_TEMPLATE, "Template for in game name of replay", true, true, 0, true, 0, true).bindTo(replayNameTemplate);
	cvarManager->getCvar(CVAR_REPLAY_NAME_TEMPLATE).addOnValueChanged([this](string oldVal, CVarWrapper cvar)
	{
		if (SanitizeReplayNameTemplate(replayNameTemplate, DEFAULT_REPLAY_NAME_TEMPLATE))
		{
			cvarManager->getCvar(CVAR_REPLAY_NAME_TEMPLATE).setValue(*replayNameTemplate);
		}
	});

	// Path to export replays to
	cvarManager->registerCvar(CVAR_REPLAY_EXPORT, "0", "Save all replay files to export filepath above.", true, true, 0, true, 1).bindTo(saveReplay);
	cvarManager->registerCvar(CVAR_REPLAY_EXPORT_PATH, DEAULT_EXPORT_PATH, "Path to export replays to.").bindTo(exportPath);
	cvarManager->getCvar(CVAR_REPLAY_EXPORT_PATH).addOnValueChanged([this](string oldVal, CVarWrapper cvar)
	{
		if (SanitizeExportPath(exportPath, DEAULT_EXPORT_PATH))
		{
			cvarManager->getCvar(CVAR_REPLAY_EXPORT_PATH).setValue(*exportPath);
		}
	});

#ifdef TOAST
	// Notification variables
	cvarManager->registerCvar(CVAR_PLUGIN_SHOW_NOTIFICATIONS, "1", "Show notifications on successful uploads", true, true, 0, true, 1).bindTo(showNotifications);
#endif
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
		return;
	}

	// Get Replay wrapper
	ReplaySoccarWrapper soccarReplay = replayDirector.GetReplay();
	if (soccarReplay.memory_address == NULL)
	{
		cvarManager->log("Could not upload replay, replay is NULL!");
		return;
	}

	// If we have a template for the replay name then set the replay name based off that template else use default template
	string replayName = SetReplayName(caller, soccarReplay);

	// Export the replay to a file for upload
	string replayPath = ExportReplay(soccarReplay, replayName);

	// Upload replay
	if (*uploadToCalculated)
	{
		calculated->UploadReplay(replayPath, replayName, to_string(caller.GetLocalPrimaryPlayer().GetPRI().GetUniqueId().ID));
	}
	if (*uploadToBallchasing)
	{
		ballchasing->UploadReplay(replayPath, replayName);
	}

	// If we aren't saving the replay remove it after we've uploaded
	if ((*saveReplay) == false)
	{
		cvarManager->log("Removing replay file: " + replayPath);
		remove(replayPath.c_str());
	}
#ifdef TOAST
	else if(*showNotifications)
	{
		bool exported = file_exists(replayPath);
		string msg = exported ? "Exported replay to: " + replayPath : "Failed to export replay to: " + replayPath;
		gameWrapper->Toast("Autoreplayuploader", msg, "default", 3.5f, exported ? ToastType_OK : ToastType_Error);
	}
#endif
}

Player ConstructPlayer(PriWrapper wrapper)
{
	Player p;
	if (!wrapper.IsNull()) 
	{
		p.Name = wrapper.GetPlayerName().ToString();
		p.UniqueId = wrapper.GetUniqueId().ID;
		p.Team = wrapper.GetTeamNum();
		p.Score = wrapper.GetScore();
		p.Goals = wrapper.GetMatchGoals();
		p.Assists = wrapper.GetMatchAssists();
		p.Saves = wrapper.GetMatchSaves();
		p.Shots = wrapper.GetMatchShots();
		p.Demos = wrapper.GetMatchDemolishes();
	}
	return p;
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
string AutoReplayUploaderPlugin::SetReplayName(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay)
{
	string replayName = *replayNameTemplate;
	cvarManager->log("Using replay name template: " + replayName);

	Match match;

	// Get Gamemode game was in
	auto playlist = server.GetPlaylist();
	if (!playlist.memory_address != NULL)
	{
		match.GameMode = GetPlaylistName(playlist.GetPlaylistId());
	}
	// Get local primary player
	auto localPlayer = server.GetLocalPrimaryPlayer();
	if (!localPlayer.IsNull())
	{
		match.PrimaryPlayer = ConstructPlayer(localPlayer.GetPRI());
	}

	// Get all players
	auto players = server.GetLocalPlayers();
	for (int i = 0; i < players.Count(); i++)
	{
		match.Players.push_back(ConstructPlayer(players.Get(i).GetPRI()));
	}

	// Get Team scores
	match.Team0Score = soccarReplay.GetTeam0Score();
	match.Team1Score = soccarReplay.GetTeam1Score();

	// Get current Sequence number
	auto seq = *templateSequence;

	replayName = ApplyNameTemplate(replayName, match, &seq);

	// Did sequence number change if so update setting
	if (seq != *templateSequence)
	{
		*templateSequence = seq;
		cvarManager->getCvar(CVAR_REPLAY_SEQUENCE_NUM).setValue(seq);
		cvarManager->executeCommand("writeconfig"); // since we change this variable ourselves we want to write the config when it changes so it persists across loads
	}

	cvarManager->log("ReplayName: " + replayName);
	soccarReplay.SetReplayName(replayName);

	return replayName;
}

string AutoReplayUploaderPlugin::ExportReplay(ReplaySoccarWrapper& soccarReplay, string replayName)
{
	string replayPath = CalculateReplayPath(*exportPath, replayName);

	// Remove file if it already exists
	if (file_exists(replayPath))
	{
		cvarManager->log("Removing duplicate replay file: " + replayPath);
		remove(replayPath.c_str());
	}

	// Export Replay
	soccarReplay.ExportReplay(replayPath);
	cvarManager->log("Exported replay to: " + replayPath);

	// Check to see if replay exists, if not then export to default path
	if (!file_exists(replayPath))
	{
		cvarManager->log("Export failed to path: " + replayPath + " exporting to default path.");
		replayPath = string(DEAULT_EXPORT_PATH) + "/autosaved.replay";

		soccarReplay.ExportReplay(replayPath);
		cvarManager->log("Exported replay to: " + replayPath);
	}

	return replayPath;
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