#include "Plugin.h"
#include "Replay.h"

using namespace std;

static inline bool file_exists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

Plugin::Plugin(void(*Log)(void* object, string message), void* Client, IReplayUploader* ballchasing, IReplayUploader* calculated)
{
	this->Log = Log;
	this->Client = Client;
	this->ballchasing = ballchasing;
	this->calculated = calculated;
	this->needToUploadReplay = false;
}

string Plugin::SetReplayName(IReplay* replay, Match& match)
{
	string replayName = *replayNameTemplate;
	this->Log(this->Client, "Using replay name template: " + replayName);

	// Get current Sequence number
	auto seq = *templateSequence;

	replayName = ApplyNameTemplate(replayName, match, &seq);

	// Did sequence number change if so update setting
	if (seq != *templateSequence)
	{
		*templateSequence = seq;
	}

	this->Log(this->Client, "ReplayName: " + replayName);
	replay->SetReplayName(replayName);

	return replayName;
}

string Plugin::ExportReplay(IReplay* replay, string& replayName)
{
	string replayPath = CalculateReplayPath(*exportPath, replayName);

	// Remove file if it already exists
	if (file_exists(replayPath))
	{
		this->Log(this->Client, "Removing duplicate replay file: " + replayPath);
		remove(replayPath.c_str());
	}

	// Export Replay
	replay->ExportReplay(replayPath);
	this->Log(this->Client, "Exported replay to: " + replayPath);

	// Check to see if replay exists, if not then export to default path
	if (!file_exists(replayPath))
	{
		this->Log(this->Client, "Export failed to path: " + replayPath + " exporting to default path.");
		replayPath = string(DEAULT_EXPORT_PATH) + "autosaved.replay";

		replay->ExportReplay(replayPath);
		this->Log(this->Client, "Exported replay to: " + replayPath);
	}

	return replayPath;
}

void Plugin::OnGameComplete(string eventName, void* serverWrapper, IReplay* (*GetReplay)(void* serverWrapper, void(*Log)(void* object, string message), void* object), Match(*GetMatch)(void* serverWrapper, IReplay* replay))
{
	if (!*this->uploadToCalculated && !*this->uploadToBallchasing) // Bail if we aren't uploading replays
	{
		return; //Not uploading replays
	}

	if (this->needToUploadReplay == false) {
		// Replay might have already been saved by Function TAGame.GameEvent_Soccar_TA.EventMatchEnded
		// event if the player stayed in the game long enough. Or we are leaving freeplay, 
		// custom training, etc instead of online game and will not proceed to upload anything.
		return;
	}
	else {
		// Since the needToUploadReplay was true, we have just finished an online game and this is
		// the first time uploading is requested. We will flag that the upload process has 
		// now been started and will continue to upload the replay.
		this->needToUploadReplay = false;
		this->Log(this->Client, "Uploading replay started: " + eventName);
	}

	IReplay* replay = GetReplay(serverWrapper, this->Log, this->Client);
	Match match = GetMatch(serverWrapper, replay);

	// If upload game was initiated by event Function TAGame.GameEvent_Soccar_TA.Destroyed
	// it is very likely that the primary player data can not be anymore fetched.
	// That's why we saved this data in Function GameEvent_Soccar_TA.Active.StartRound -event
	// and will use it, if needed, to get correct player for the game being uploaded.
	if (match.PrimaryPlayer.Name.length() < 1 && this->BackupPlayer.Name.length() > 0)
	{
		this->Log(this->Client, "Using prerecorder username for replay: " + this->BackupPlayer.Name);
		match.PrimaryPlayer.Name = this->BackupPlayer.Name;
		match.PrimaryPlayer.UniqueId = this->BackupPlayer.UniqueId;
		match.PrimaryPlayer.Team = this->BackupPlayer.Team;
	}

	string replayName = this->SetReplayName(replay, match);
	string replayPath = this->ExportReplay(replay, replayName);

	// Upload replay
	string playerId = to_string(match.PrimaryPlayer.UniqueId);
	if (*uploadToCalculated)
	{
		calculated->UploadReplay(replayPath, playerId);
	}
	if (*uploadToBallchasing)
	{
		ballchasing->UploadReplay(replayPath, playerId);
	}

	if ((*saveReplay) == false)
	{
		this->Log(this->Client, "Removing replay file: " + replayPath);
		remove(replayPath.c_str());
	}
}

void Plugin::GetPlayerData(string eventName, void* gameWrapper, bool isOnlineGame, Player(*GetPlayer)(void* gameWrapper))
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
	if (isOnlineGame) 
	{
		this->needToUploadReplay = true;
	}
	else {
		//If we are not in online game, we are in freeplay, custom training etc
		// We will set the needToUploadReplay flag to false and no need to save the player data.
		this->needToUploadReplay = false;
		return;
	}

	this->BackupPlayer = GetPlayer(gameWrapper);

	this->Log(this->Client, "StartRound: Stored userdata for:" + this->BackupPlayer.Name);
}