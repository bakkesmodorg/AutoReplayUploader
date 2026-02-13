#include "AutoReplayUploaderPlugin.h"

#include <chrono>
#include <sstream>
#include "utils/io.h"

#include "bakkesmod/wrappers/GameEvent/ReplayWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplayDirectorWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplaySoccarWrapper.h"

#include "Utils.h"
#include "Ballchasing.h"
#include "Calculated.h"
#include "Match.h"
#include "Player.h"
#include "Replay.h"

#include "bakkesmod/wrappers/wrapperstructs.h"

BAKKESMOD_PLUGIN(AutoReplayUploaderPlugin, "Auto replay uploader plugin", "0.2", 0);

// Constant CVAR variable names
#define CVAR_REPLAY_EXPORT_PATH "cl_autoreplayupload_filepath"
#define CVAR_REPLAY_EXPORT "cl_autoreplayupload_save"
#define CVAR_UPLOAD_TO_CALCULATED "cl_autoreplayupload_calculated"
#define CVAR_UPLOAD_TO_BALLCHASING "cl_autoreplayupload_ballchasing"
#define CVAR_UPLOAD_TO_BALLCHASING_MMR "cl_autoreplayupload_ballchasing_mmr"
#define CVAR_REPLAY_NAME_TEMPLATE "cl_autoreplayupload_replaynametemplate"
#define CVAR_REPLAY_SEQUENCE_NUM "cl_autoreplayupload_replaysequence"
#define CVAR_PLUGIN_SHOW_NOTIFICATIONS "cl_autoreplayupload_notifications"
#define CVAR_BALLCHASING_AUTH_KEY "cl_autoreplayupload_ballchasing_authkey"
#define CVAR_BALLCHASING_AUTH_TEST_RESULT "cl_autoreplayupload_ballchasing_testkeyresult"
#define CVAR_BALLCHASING_REPLAY_VISIBILITY "cl_autoreplayupload_ballchasing_visibility"
#define CVAR_CALCULATED_REPLAY_VISIBILITY "cl_autoreplayupload_calculated_visibility"
#define CVAR_UPLOAD_ONLINE_FREEPLAY "cl_autoreplayupload_online_freeplay"

Player ConstructPlayer(PriWrapper wrapper)
{
	Player p;
	if (!wrapper.IsNull())
	{
		p.Name = wrapper.GetPlayerName().ToString();
		p.UniqueId = wrapper.GetUniqueIdWrapper().GetUID();
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

std::string GetPlaylistName(int playlistId);
Match backupMatchForReplayName;
bool needToUploadReplay = false;
std::string backupPlayerSteamID = "";
std::chrono::time_point<std::chrono::steady_clock> pluginLoadTime;

struct MMRCacheWrapper {
	MMRData data;
	bool addToAfter;

	void AddPlayer(std::string name, UniqueIDWrapper id, float mmr, SkillRank extra, int platform) {
		if (mmr < 101) return;
		MMR newValue{ extra.Tier, extra.Division, extra.MatchesPlayed, mmr };
		std::string sid = std::to_string(id.GetUID());
		PlayerMMRData newPlayerData{ platform, sid, newValue };
		newPlayerData.debug = name;
		auto it = std::find_if(data.players.begin(), data.players.end(), [&](PlayerMMRData& p) { return p.id == sid; });
		if (it == data.players.end()) {
			data.players.push_back(newPlayerData);
		}
		else {
			auto index = std::distance(data.players.begin(), it);
			if (addToAfter) {
				data.players[index].hasAfter = true;
				data.players[index].after = newValue;
			}
			else {
				data.players[index] = newPlayerData;
			}
		}
	}

	void Clear() {
		data.players.clear();
		data.game = "";
		addToAfter = false;
	}

	void OnMatchEnd() {
		addToAfter = true;
	}

	void SetMatchGuid(ServerWrapper server) {
		this->data.game = server.GetMatchGUID();
	}
};
MMRCacheWrapper mmrCache;


void Log(void* object, std::string message)
{
	auto plugin = (AutoReplayUploaderPlugin*)object;
	plugin->cvarManager->log(message);
}

void UploadComplete(AutoReplayUploaderPlugin* plugin, bool result, std::string endpoint)
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
	std::string msg = result ? "Auth key correct!" : "Invalid auth key!";
	plugin->cvarManager->getCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT).setValue(msg);
}

#pragma region AutoReplayUploaderPlugin Implementation

/**
* OnLoad event called when the plugin is loaded by BakkesMod
*/
void AutoReplayUploaderPlugin::onLoad()
{
	std::stringstream userAgentStream;
	userAgentStream << exports.className << "/" << exports.pluginVersion << " BakkesModAPI/" << BAKKESMOD_PLUGIN_API_VERSION;
	std::string userAgent = userAgentStream.str();

	pluginLoadTime = std::chrono::steady_clock::now();

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
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3
		)
	);

	// Register for Game ending event	
	gameWrapper->HookEventWithCaller<ServerWrapper>(
		"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
		bind(
			&AutoReplayUploaderPlugin::OnGameComplete,
			this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3
		)
	);

	// Register for Game ending event	
	gameWrapper->HookEventWithCaller<ServerWrapper>(
		"Function TAGame.GameEvent_Soccar_TA.Destroyed",
		bind(
			&AutoReplayUploaderPlugin::OnGameComplete,
			this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3
		)
	);

	// Register for MMR updates
	gameWrapper->HookEventPost(
		//"Function ProjectX.PlaylistSkillCache_X.CacheSkill",
		"Function ProjectX.OnlineGameSkill_X.HandleSkillRequestCompleteRPC",
		std::bind(&AutoReplayUploaderPlugin::OnMMRSync, this));

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
	// Set the default status of uploading replay to false
	needToUploadReplay = false;
 
	// Default to not uploading Online Freeplay replays
	cvarManager->registerCvar(CVAR_UPLOAD_ONLINE_FREEPLAY, "0", "Upload replays from online freeplay matches", true, true, 0, true, 1).bindTo(uploadOnlineFreeplay);

	// Calculated variables
	cvarManager->registerCvar(CVAR_UPLOAD_TO_CALCULATED, "0", "Upload to replays to calculated.gg automatically", true, true, 0, true, 1).bindTo(uploadToCalculated);		
	cvarManager->registerCvar(CVAR_CALCULATED_REPLAY_VISIBILITY, "DEFAULT", "Replay visibility when uploading to calculated.gg", false, false, 0, false, 0, true).bindTo(calculated->visibility);

	// Ball Chasing variables	
	cvarManager->registerCvar(CVAR_UPLOAD_TO_BALLCHASING, "0", "Upload to replays to ballchasing.com automatically", true, true, 0, true, 1).bindTo(uploadToBallchasing);
	cvarManager->registerCvar(CVAR_UPLOAD_TO_BALLCHASING_MMR, "0", "Upload mmr data to ballchasing.com automatically", true, true, 0, true, 1).bindTo(uploadToBallchasingMMR);
	cvarManager->registerCvar(CVAR_BALLCHASING_REPLAY_VISIBILITY, "public", "Replay visibility when uploading to ballchasing.com", false, false, 0, false, 0, true).bindTo(ballchasing->visibility);
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT, "Untested", "Auth token needed to upload replays to ballchasing.com", false, false, 0, false, 0, false);
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_KEY, "", "Auth token needed to upload replays to ballchasing.com").bindTo(ballchasing->authKey);
	cvarManager->getCvar(CVAR_BALLCHASING_AUTH_KEY).addOnValueChanged([this](std::string oldVal, CVarWrapper cvar)
	{   
		if (ballchasing->authKey->size() > 0 &&         // We don't test the auth key if the size of the auth key is empty
			ballchasing->authKey->compare(oldVal) != 0) // We don't test unless the value has changed
		{
			auto elapsed = std::chrono::steady_clock::now() - pluginLoadTime;
			if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed) < std::chrono::milliseconds(5000))
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
	cvarManager->getCvar(CVAR_REPLAY_NAME_TEMPLATE).addOnValueChanged([this](std::string oldVal, CVarWrapper cvar)
	{
		if (SanitizeReplayNameTemplate(replayNameTemplate, DEFAULT_REPLAY_NAME_TEMPLATE))
		{
			cvarManager->getCvar(CVAR_REPLAY_NAME_TEMPLATE).setValue(*replayNameTemplate);
		}
	});

	// Path to export replays to
	cvarManager->registerCvar(CVAR_REPLAY_EXPORT, "0", "Save all replay files to export filepath above.", true, true, 0, true, 1).bindTo(saveReplay);
	cvarManager->registerCvar(CVAR_REPLAY_EXPORT_PATH, DEAULT_EXPORT_PATH, "Path to export replays to.").bindTo(exportPath);
	cvarManager->getCvar(CVAR_REPLAY_EXPORT_PATH).addOnValueChanged([this](std::string oldVal, CVarWrapper cvar)
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
void AutoReplayUploaderPlugin::OnGameComplete(ServerWrapper caller, void * params, std::string eventName)
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
	std::string replayName = SetReplayName(caller, soccarReplay);
	

	// Export the replay to a file for upload
	std::string replayPath = ExportReplay(soccarReplay, replayName);

	// If we are saving this with event Function TAGame.GameEvent_Soccar_TA.Destroyed
	// the steamID might not be available. Using prestored steamID
	std::string playerSteamID = std::to_string(gameWrapper->GetSteamID());
    if (playerSteamID.length() < 1) {
    	playerSteamID = backupPlayerSteamID;
		cvarManager->log("Using backup steamId to upload: " + playerSteamID);
    } else {
		cvarManager->log("Using steamId to upload: " + playerSteamID);
    }

	// Upload replay
	if (*uploadToCalculated)
	{
		calculated->UploadReplay(replayPath, playerSteamID);
	}
	if (*uploadToBallchasing)
	{
		ballchasing->UploadReplay(replayPath);
		if (*uploadToBallchasingMMR) {
			auto server = gameWrapper->GetOnlineGame();
			if (server) {
				mmrCache.SetMatchGuid(server);
				mmrCache.addToAfter = true;
				gameWrapper->SetTimeout([&](auto gw) {
					cvarManager->log("Uploading with match id: " + mmrCache.data.game);
					cvarManager->log("the cache contains " + std::to_string(mmrCache.data.players.size()) + " players");
					for (auto p : mmrCache.data.players) {
						cvarManager->log("\t" + p.debug);
					}
					ballchasing->UploadMMr(mmrCache.data);
					mmrCache.Clear();
				}, 3);
			}
		}
		else {
			mmrCache.Clear();
		}
		
	}

	// If we aren't saving the replay remove it after we've uploaded
	if (!(*saveReplay))
	{
		cvarManager->log("Removing replay file: " + replayPath);
		remove(replayPath.c_str());
	}
#ifdef TOAST
	else if(*showNotifications)
	{
		bool exported = file_exists(replayPath);
		std::string msg = exported ? "Exported replay to: " + replayPath : "Failed to export replay to: " + replayPath;
		gameWrapper->Toast("Autoreplayuploader", msg, "default", 3.5f, exported ? ToastType_OK : ToastType_Error);
	}
#endif
}

struct Res {
	std::string name;
	float mmr;
};

void AutoReplayUploaderPlugin::OnMMRSync()
{
	auto mmrWrapper = gameWrapper->GetMMRWrapper();
	auto playlist = mmrWrapper.GetCurrentPlaylist();
	auto server = gameWrapper->GetOnlineGame();
	if (!server) {
		//cvarManager->log("no server");
		return;
	}
	cvarManager->log("on mmr sync");
	//cvarManager->log("current playlist is: " + std::to_string(playlist));
	//cvarManager->log("Getting the players");
	std::vector<Res> res;
	auto players = server.GetPRIs();
	cvarManager->log("got " + std::to_string(players.Count()) + " players");
	for (size_t i = 0; i < players.Count(); i++)
	{
		auto player = players.Get(static_cast<int>(i));
		if (!player) continue;
		auto playerName = player.GetPlayerName().ToString();
		auto platform = player.GetPlatform();
		//cvarManager->log("Getting MMR for :  "+ playerName);
		auto playerSteam = player.GetUniqueIdWrapper();
		if (playerSteam.GetUID() == 0) continue; //bot
		if (!mmrWrapper.IsSyncing(playerSteam)) {
			auto mmr = mmrWrapper.GetPlayerMMR(playerSteam, playlist);
			auto extra = mmrWrapper.GetPlayerRank(playerSteam, playlist);
			mmrCache.AddPlayer(playerName, playerSteam, mmr, extra, platform);
			res.push_back({ playerName , mmr });
			//cvarManager->log(playerName + " MMR: " + std::to_string(mmr));
		}
	}
	cvarManager->log("MMR results");
	for (auto r : res) {
		cvarManager->log("\t" + r.name + ":" + std::to_string(r.mmr));
	}


}

void AutoReplayUploaderPlugin::GetPlayerData(ServerWrapper caller, void* params, std::string eventName)
{
    /*****************************************************************************************
    * This function will save primary player userdata at the start of any online game. There *
    * are now number of different events that will try to upload the replays after matches.  *
    * This is needed to make the replay uploads more reliable no matter how fast or slow you *
    * leave the current game.  In some cases the player data might not be anymore available  * 
    * when uploading the replay, and that is why we save it in at start of the match.        *
    *****************************************************************************************/
	
	// Function GameEvent_Soccar_TA.Active.StartRound -event will fire in all modes
	// like freeplay, custom training etc. Set the needToUploadReplay flag replay only 
	// if we are in an online game. Online freeplay is also an online game mode, but we will
	// only upload those replays if CVAR_UPLOAD_ONLINE_FREEPLAY (bound to uploadOnlineFreeplay)
	// has been set to true.

	int playlistID = caller.GetPlaylist().GetPlaylistId();
	needToUploadReplay = gameWrapper->IsInOnlineGame() && (playlistID != ONLINE_FREEPLAY_PLAYLIST_ID ? true : *uploadOnlineFreeplay);
	if (!needToUploadReplay) {
		return;
	}

	// We are in online game and now storing some important player data, if needed after the game
	// When uploading the replay.
	backupPlayerSteamID = std::to_string(gameWrapper->GetSteamID());

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
std::string AutoReplayUploaderPlugin::SetReplayName(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay)
{
	std::string replayName = *replayNameTemplate;
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

std::string AutoReplayUploaderPlugin::ExportReplay(ReplaySoccarWrapper& soccarReplay, std::string replayName)
{
	std::string replayPath = CalculateReplayPath(*exportPath, replayName);

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
		replayPath = std::string(DEAULT_EXPORT_PATH) + "autosaved.replay";

		soccarReplay.ExportReplay(replayPath);
		cvarManager->log("Exported replay to: " + replayPath);
	}

	return replayPath;
}

#pragma endregion

#pragma region Utility Functions

std::string GetPlaylistName(int playlistId) {
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
