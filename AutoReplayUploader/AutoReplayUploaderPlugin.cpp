#include "AutoReplayUploaderPlugin.h"

#include <sstream>

#include "bakkesmod/wrappers/GameEvent/ReplayWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplayDirectorWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ReplaySoccarWrapper.h"

#include "Match.h"
#include "Player.h"
#include "Replay.h"
#include "IReplay.h"
#include "Plugin.h"

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

void Log(void* object, string message)
{
	auto plugin = (AutoReplayUploaderPlugin*)object;
	plugin->cvarManager->log(message);
}

void UploadComplete(void* object, bool result, string endpoint)
{
	AutoReplayUploaderPlugin* plugin = (AutoReplayUploaderPlugin*)object;
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

	// Setup plugin
	plugin = new Plugin(&Log, this, (IReplayUploader*)ballchasing, (IReplayUploader*)calculated);

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
	gameWrapper->LoadToastTexture("calculated_logo", "./bakkesmod/data/assets/calculated_logo.tga");
	gameWrapper->LoadToastTexture("ballchasing_logo", "./bakkesmod/data/assets/ballchasing_logo.tga");
}

/**
* OnUnload event called when the plugin is unloaded by BakkesMod
*/
void AutoReplayUploaderPlugin::onUnload()
{
	delete ballchasing;
	delete calculated;
	delete plugin;
}

void AutoReplayUploaderPlugin::InitializeVariables()
{
	// Calculated variables
	cvarManager->registerCvar(CVAR_UPLOAD_TO_CALCULATED, "0", "Upload to replays to calculated.gg automatically", true, true, 0, true, 1).bindTo(plugin->uploadToBallchasing);		
	cvarManager->registerCvar(CVAR_CALCULATED_REPLAY_VISIBILITY, CALCULATED_DEFAULT_REPLAY_VISIBILITY, "Replay visibility when uploading to calculated.gg", false, false, 0, false, 0, true).bindTo(calculated->visibility);

	// Ball Chasing variables	
	cvarManager->registerCvar(CVAR_UPLOAD_TO_BALLCHASING, "0", "Upload to replays to ballchasing.com automatically", true, true, 0, true, 1).bindTo(plugin->uploadToBallchasing);
	cvarManager->registerCvar(CVAR_BALLCHASING_REPLAY_VISIBILITY, BALLCHASING_DEFAULT_REPLAY_VISIBILITY, "Replay visibility when uploading to ballchasing.com", false, false, 0, false, 0, true).bindTo(ballchasing->visibility);
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_TEST_RESULT, BALLCHASING_DEFAULT_AUTH_TEST_RESULT, "Auth token needed to upload replays to ballchasing.com", false, false, 0, false, 0, false);
	cvarManager->registerCvar(CVAR_BALLCHASING_AUTH_KEY, BALLCHASING_DEFAULT_AUTH_KEY, "Auth token needed to upload replays to ballchasing.com").bindTo(ballchasing->authKey);
	cvarManager->getCvar(CVAR_BALLCHASING_AUTH_KEY).addOnValueChanged([this](string oldVal, CVarWrapper cvar)
	{   
		ballchasing->OnBallChasingAuthKeyChanged(oldVal);
	});

	// Replay Name template variables
	cvarManager->registerCvar(CVAR_REPLAY_SEQUENCE_NUM, "0", "Current Reqlay Sequence number to be used in replay name", true, true, 0, false, 0, true).bindTo(plugin->templateSequence);
	cvarManager->registerCvar(CVAR_REPLAY_NAME_TEMPLATE, DEFAULT_REPLAY_NAME_TEMPLATE, "Template for in game name of replay", true, true, 0, true, 0, true).bindTo(plugin->replayNameTemplate);
	cvarManager->getCvar(CVAR_REPLAY_NAME_TEMPLATE).addOnValueChanged([this](string oldVal, CVarWrapper cvar)
	{
		if (SanitizeReplayNameTemplate(plugin->replayNameTemplate, DEFAULT_REPLAY_NAME_TEMPLATE))
		{
			cvarManager->getCvar(CVAR_REPLAY_NAME_TEMPLATE).setValue(*(plugin->replayNameTemplate));
		}
	});

	// Path to export replays to
	cvarManager->registerCvar(CVAR_REPLAY_EXPORT, "0", "Save all replay files to export filepath above.", true, true, 0, true, 1).bindTo(plugin->saveReplay);
	cvarManager->registerCvar(CVAR_REPLAY_EXPORT_PATH, DEAULT_EXPORT_PATH, "Path to export replays to.").bindTo(plugin->exportPath);
	cvarManager->getCvar(CVAR_REPLAY_EXPORT_PATH).addOnValueChanged([this](string oldVal, CVarWrapper cvar)
	{
		if (SanitizeExportPath(plugin->exportPath, DEAULT_EXPORT_PATH))
		{
			cvarManager->getCvar(CVAR_REPLAY_EXPORT_PATH).setValue(*(plugin->exportPath));
		}
	});

	// Notification variables
	cvarManager->registerCvar(CVAR_PLUGIN_SHOW_NOTIFICATIONS, "1", "Show notifications on successful uploads", true, true, 0, true, 1).bindTo(showNotifications);
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

Match GetMatch(void* serverWrapper, IReplay* replay)
{
	auto server = (ServerWrapper*)serverWrapper;
	Match match;

	// Get Gamemode game was in
	auto playlist = server->GetPlaylist();
	if (playlist.memory_address != NULL)
	{
		match.GameMode = GetPlaylistName(playlist.GetPlaylistId());
	}
	// Get local primary player
	CarWrapper mycar = server->GetLocalPrimaryPlayer().GetCar();
	if (!mycar.IsNull())
	{
		PriWrapper mycarpri = mycar.GetPRI();
		if (!mycarpri.IsNull())
		{
			match.PrimaryPlayer = ConstructPlayer(mycarpri);
		}
	}

	// Get all players
	auto players = server->GetLocalPlayers();
	for (int i = 0; i < players.Count(); i++)
	{
		match.Players.push_back(ConstructPlayer(players.Get(i).GetPRI()));
	}

	// Get Team scores
	match.Team0Score = replay->GetTeam0Score();
	match.Team1Score = replay->GetTeam1Score();

	return match;
}

Player GetPrimaryPlayer(void* gameWrapper)
{
	auto server = (GameWrapper*)gameWrapper;
	CarWrapper mycar = server->GetLocalCar();
	if (!mycar.IsNull())
	{
		PriWrapper mycarpri = mycar.GetPRI();
		if (!mycarpri.IsNull())
		{
			return ConstructPlayer(mycarpri);
		}
	}
	return Player();
}

class WrappedReplay : IReplay
{
private:
	ReplaySoccarWrapper* replay;

public:

	WrappedReplay(ReplaySoccarWrapper* wrapper)
	{
		replay = wrapper;
	}

	virtual int GetTeam0Score() { return replay->GetTeam0Score(); }
	virtual int GetTeam1Score() { return replay->GetTeam1Score(); }
	virtual void ExportReplay(string replayPath) { replay->ExportReplay(replayPath); };
	virtual void SetReplayName(string replayName) { replay->SetReplayName(replayName);  };
};

IReplay* GetReplay(void* serverWrapper, void(*Log)(void* object, string message), void* object)
{
	ServerWrapper* caller = (ServerWrapper*)serverWrapper;

	// Get ReplayDirector
	ReplayDirectorWrapper replayDirector = caller->GetReplayDirector();
	if (replayDirector.IsNull())
	{
		Log(object, "Could not upload replay, director is NULL!");
		return nullptr;
	}

	// Get Replay wrapper
	ReplaySoccarWrapper soccarReplay = replayDirector.GetReplay();
	if (soccarReplay.memory_address == NULL)
	{
		Log(object, "Could not upload replay, replay is NULL!");
		return nullptr;
	}
	soccarReplay.StopRecord();
	return (IReplay*)new WrappedReplay(&soccarReplay);
}

void AutoReplayUploaderPlugin::OnGameComplete(ServerWrapper caller, void* params, string eventName)
{
	plugin->OnGameComplete(eventName, (void*)&caller, GetReplay, GetMatch);
}

void AutoReplayUploaderPlugin::GetPlayerData(ServerWrapper caller, void* params, string eventName)
{
	plugin->GetPlayerData(eventName, (void*)&caller, gameWrapper->IsInOnlineGame(), GetPrimaryPlayer);
}