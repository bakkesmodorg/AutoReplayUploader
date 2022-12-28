#include "AutoReplayUploaderPlugin.h"

#include <chrono>
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

BAKKESMOD_PLUGIN(AutoReplayUploaderPlugin, "Auto replay uploader plugin", "0.2", 0);

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
Match backupMatchForReplayName;
bool needToUploadReplay = false;
string backupPlayerSteamID = "";
std::chrono::time_point<std::chrono::steady_clock> pluginLoadTime;

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

	pluginLoadTime = chrono::steady_clock::now();

	// Setup upload handlers
	ballchasing = new Ballchasing(userAgent, &Log, &BallchasingUploadComplete, &BallchasingAuthTestComplete, this);
	calculated = new Calculated(userAgent, &Log, &CalculatedUploadComplete, this);

	InitializeVariables();


	// Register for Game ending event	
	gameWrapper->HookEventWithCaller<ServerWrapper>(
		"Function GameEvent_Soccar_TA.Active.StartRound",
		bind(
			&AutoReplayUploaderPlugin::GetPlayerData,
			this,
			placeholders::_1,
			placeholders::_2,
			placeholders::_3
		)
	);

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

	// Register for Game ending event	
	gameWrapper->HookEventWithCaller<ServerWrapper>(
		"Function TAGame.GameEvent_Soccar_TA.Destroyed",
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
	gameWrapper->LoadToastTexture("calculated_logo", gameWrapper->FixRelativePath("./bakkesmod/data/assets/calculated_logo.tga"));
	gameWrapper->LoadToastTexture("ballchasing_logo", gameWrapper->FixRelativePath("./bakkesmod/data/assets/ballchasing_logo.tga"));
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
	// Set the default status of uploading replay to false
	needToUploadReplay = false;
 
	// Calculated variables
	cvarManager->registerCvar(CVAR_UPLOAD_TO_CALCULATED, "0", "Upload to replays to calculated.gg automatically", true, true, 0, true, 1).bindTo(uploadToCalculated);		
	cvarManager->registerCvar(CVAR_CALCULATED_REPLAY_VISIBILITY, "DEFAULT", "Replay visibility when uploading to calculated.gg", false, false, 0, false, 0, true).bindTo(calculated->visibility);

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
			auto elapsed = chrono::steady_clock::now() - pluginLoadTime;
			if (chrono::duration_cast<chrono::milliseconds>(elapsed) < chrono::milliseconds(5000))
			{
				cvarManager->log("Not checking auth key since plugin was loaded recently");
			}
			else
			{
				// value changed so test auth key
				ballchasing->TestAuthKey();
			}
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
	cvarManager->registerCvar(CVAR_REPLAY_EXPORT_PATH, DEFAULT_EXPORT_PATH, "Path to export replays to.").bindTo(exportPath);
	cvarManager->getCvar(CVAR_REPLAY_EXPORT_PATH).addOnValueChanged([this](string oldVal, CVarWrapper cvar)
	{
		if (SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH))
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
	if (!(*uploadToCalculated) && !(*uploadToBallchasing) && !(*saveReplay)) // Bail if we aren't uploading or saving replays
	{
		return; //Not uploading replays
	}

    if (!needToUploadReplay) {
		// Replay might have already been saved by Function TAGame.GameEvent_Soccar_TA.EventMatchEnded
		// event if the player stayed in the game long enough. Or we are leaving freeplay, 
		// custom training, etc instead of online game and will not proceed to upload anything.
    	return;
    } else {
    	// Since the needToUploadReplay was true, we have just finished an online game and this is
    	// the first time uploading is requested. We will flag that the upload process has 
    	// now been started and will continue to upload the replay.
    	needToUploadReplay = false;
		cvarManager->log("Uploading replay started: " + eventName);
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
	soccarReplay.StopRecord();
	
	// If we have a template for the replay name then set the replay name based off that template else use default template
	string replayName = SetReplayName(caller, soccarReplay);
	
	cvarManager->log("Exporting replay!");
	// Export the replay to a file for upload
	std::filesystem::path replayPath = ExportReplay(soccarReplay, replayName);
	if (!std::filesystem::exists(replayPath) || std::filesystem::is_directory(replayPath))
	{
		cvarManager->log("Failed exporting replay to " + replayPath.string());
		return;
	}

	
    UniqueIDWrapper playerSteamID = gameWrapper->GetUniqueID();
	std::string uploadID = std::to_string(playerSteamID.GetUID());
    if (uploadID.size() < 1) {
		uploadID = playerSteamID.str();
		cvarManager->log("Using backup steamId to upload: " + uploadID);
    } else {
		cvarManager->log("Using steamId to upload: " + uploadID);
    }

	// Upload replay
	if (*uploadToCalculated)
	{
		calculated->UploadReplay(gameWrapper->GetBakkesModPath(), replayPath, uploadID);
	}
	if (*uploadToBallchasing)
	{
		ballchasing->UploadReplay(gameWrapper->GetBakkesModPath(), replayPath);
	}

	// If we aren't saving the replay remove it after we've uploaded
	if (!(*saveReplay))
	{
		cvarManager->log("Removing replay file: " + replayPath.string());
		std::filesystem::remove(replayPath);
	}
#ifdef TOAST
	else if(*showNotifications)
	{
		bool exported = std::filesystem::exists(replayPath);
		string msg = exported ? "Exported replay to: " + replayPath.string() : "Failed to export replay to: " + replayPath.string();
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
		p.UniqueId = wrapper.GetUniqueIdWrapper().GetUID();
		p.EpicID = wrapper.GetUniqueIdWrapper().GetEpicAccountID();
		
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

void AutoReplayUploaderPlugin::GetPlayerData(ServerWrapper caller, void * params, string eventName)
{
    /*****************************************************************************************
    * This function will save primary player userdata at the start of any online game. There *
    * are now number of different events that will try to upload the replays after matches.  *
    * This is needed to make the replay uploads more reliable no matter how fast or slow you *
    * leave the current game.  In some cases the player data might not be anymore available  * 
    * when uploading the replay, and that is why we save it in at start of the match.        *
    *****************************************************************************************/
	
	//Function GameEvent_Soccar_TA.Active.StartRound -event will fire in all modes
	//like freeplay, custom training etc. Set the needToUploadReplay flag replay only 
	//if we are in an online game.
	if (gameWrapper->IsInOnlineGame()) {
	    needToUploadReplay = true;
	} else {
		//If we are not in online game, we are in freeplay, custom training etc
		// We will set the needToUploadReplay flag to false and no need to save the player data.
	    needToUploadReplay = false;
	    return;
	}
	
	// We are in online game and now storing some important player data, if needed after the game
	// When uploading the replay.
	backupPlayerSteamID = to_string(gameWrapper->GetSteamID());

    CarWrapper mycar = gameWrapper->GetLocalCar();
    if (!mycar.IsNull()) {
        PriWrapper mycarpri = mycar.GetPRI();
        if (!mycarpri.IsNull()) {
               backupMatchForReplayName.PrimaryPlayer = ConstructPlayer(mycarpri);
        }
    }

	cvarManager->log("StartRound: Stored userdata for:" + backupMatchForReplayName.PrimaryPlayer.Name);
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
	if (playlist.memory_address != NULL)
	{
		match.GameMode = GetPlaylistName(playlist.GetPlaylistId());
	}
	// Get local primary player
	CarWrapper mycar = gameWrapper->GetLocalCar();
	if (!mycar.IsNull()) 
	{
		PriWrapper mycarpri = mycar.GetPRI();
		if (!mycarpri.IsNull()) 
		{
			match.PrimaryPlayer = ConstructPlayer(mycarpri);
		}
	}

	// If upload game was initiated by event Function TAGame.GameEvent_Soccar_TA.Destroyed
	// it is very likely that the primary player data can not be anymore fetched.
	// That's why we saved this data in Function GameEvent_Soccar_TA.Active.StartRound -event
	// and will use it, if needed, to get correct player for the game being uploaded.
	if (match.PrimaryPlayer.Name.length() < 1 && backupMatchForReplayName.PrimaryPlayer.Name.length() > 0)
	{
		cvarManager->log("Using prerecorder username for replay: " + backupMatchForReplayName.PrimaryPlayer.Name);
		match.PrimaryPlayer.Name = backupMatchForReplayName.PrimaryPlayer.Name;
		match.PrimaryPlayer.UniqueId = backupMatchForReplayName.PrimaryPlayer.UniqueId;
		match.PrimaryPlayer.EpicID = backupMatchForReplayName.PrimaryPlayer.EpicID;
		match.PrimaryPlayer.Team = backupMatchForReplayName.PrimaryPlayer.Team;
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
	
	if (auto matchWinner = server.GetMatchWinner(); !matchWinner.IsNull())
	{
		match.WinningTeam = matchWinner.GetTeamIndex();
	}
	else if (auto gameWinner = server.GetGameWinner(); !gameWinner.IsNull())
	{
		match.WinningTeam = gameWinner.GetTeamIndex();
	}
	else
	{
		match.WinningTeam = match.Team0Score > match.Team1Score ? 0 : 1;
	}


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

std::filesystem::path AutoReplayUploaderPlugin::ExportReplay(ReplaySoccarWrapper& soccarReplay, string replayName)
{
	std::filesystem::path replayPath = CalculateReplayPath(*exportPath, replayName);
	cvarManager->log("Calculated replay path: " + replayPath.string());
	if (replayPath.is_relative())
	{
		cvarManager->log("Path is a relative path, fixing it!");
		replayPath = gameWrapper->FixRelativePath(replayPath);
		cvarManager->log("Fixed path: " + replayPath.string());
	}
	// Remove file if it already exists
	if (std::filesystem::exists(replayPath))
	{
		cvarManager->log("Removing duplicate replay file: " + replayPath.string());
		if (std::filesystem::is_directory(replayPath))
		{
			cvarManager->log("Calculated export path is somehow a directory. " + replayPath.string());
			return replayPath / "err.replay";
		}
		std::filesystem::remove(replayPath);
		cvarManager->log("File removed.");
	}

	// Export Replay
	
	soccarReplay.ExportReplay(replayPath);
	cvarManager->log("Exported replay to: " + replayPath.string());

	// Check to see if replay exists, if not then export to default path
	if (!std::filesystem::exists(replayPath))
	{
		cvarManager->log("Export failed to path: " + replayPath.string() + " exporting to default path.");
		replayPath = gameWrapper->GetDataFolder() / "autosaved.replay";

		soccarReplay.ExportReplay(replayPath);
		cvarManager->log("Exported replay to: " + replayPath.string());
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
