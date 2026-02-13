#pragma once
#include <string>
#include <vector>

#include "bakkesmod/plugin/bakkesmodplugin.h"

#include "Ballchasing.h"
#include "Calculated.h"


#define DEAULT_EXPORT_PATH "./bakkesmod/data/"
#define DEFAULT_REPLAY_NAME_TEMPLATE "{YEAR}-{MONTH}-{DAY}.{HOUR}.{MIN} {PLAYER} {MODE} {WINLOSS}"
#define ONLINE_FREEPLAY_PLAYLIST_ID 73

// TODO: uncomment or remove #ifdef's when new Bakkes mod API becomes available that has Toast notifications
//#define TOAST

class AutoReplayUploaderPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	// Upload handlers
	Ballchasing* ballchasing;
	Calculated* calculated;

	// Which endpoints to upload to
	std::shared_ptr<bool> uploadToCalculated = std::make_shared<bool>(false);
	std::shared_ptr<bool> uploadToBallchasing = std::make_shared<bool>(false);
	std::shared_ptr<bool> uploadToBallchasingMMR = std::make_shared<bool>(false);
	std::shared_ptr<bool> uploadOnlineFreeplay = std::make_shared<bool>(false);

	// Replay name template variables
	std::shared_ptr<int> templateSequence = std::make_shared<int>(0);
	std::shared_ptr<std::string> replayNameTemplate = std::make_shared<std::string>(DEFAULT_REPLAY_NAME_TEMPLATE);
	
	// Export replay variables
	std::shared_ptr<bool> saveReplay = std::make_shared<bool>(false);
	std::shared_ptr<std::string> exportPath = std::make_shared<std::string>(DEAULT_EXPORT_PATH);

	// Initializes all variables from bakkes mod settings menu
	void InitializeVariables();

	std::string SetReplayName(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay);
	std::string ExportReplay(ReplaySoccarWrapper& soccarReplay, std::string replayName);

public:
	virtual void onLoad();
	virtual void onUnload();
	
	void GetPlayerData(ServerWrapper caller, void* params, std::string eventName);
	void OnGameComplete(ServerWrapper caller, void* params, std::string eventName);
	void OnMMRSync();

#ifdef TOAST
	std::shared_ptr<bool> showNotifications = std::make_shared<bool>(true);
#endif
};