#pragma once
#include <string>
#include <vector>

#include "bakkesmod/plugin/bakkesmodplugin.h"

#include "Ballchasing.h"
#include "Calculated.h"

#pragma comment( lib, "bakkesmod.lib" )

using namespace std;

#define DEAULT_EXPORT_PATH "./bakkesmod/data/"
#define DEFAULT_REPLAY_NAME_TEMPLATE "{YEAR}-{MONTH}-{DAY}.{HOUR}.{MIN} {PLAYER} {MODE} {WINLOSS}"

// TODO: uncomment or remove #ifdef's when new Bakkes mod API becomes available that has Toast notifications
// #define TOAST

class AutoReplayUploaderPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	// Upload handlers
	Ballchasing* ballchasing;
	Calculated* calculated;

	// Which endpoints to upload to
	shared_ptr<bool> uploadToCalculated = make_shared<bool>(false);
	shared_ptr<bool> uploadToBallchasing = make_shared<bool>(false);

	// Replay name template variables
	shared_ptr<int> templateSequence = make_shared<int>(0);
	shared_ptr<string> replayNameTemplate = make_shared<string>(DEFAULT_REPLAY_NAME_TEMPLATE);
	
	// Export replay variables
	shared_ptr<bool> saveReplay = make_shared<bool>(false);
	shared_ptr<string> exportPath = make_shared<string>(DEAULT_EXPORT_PATH);

#ifdef TOAST
	shared_ptr<bool> showNotifications = make_shared<bool>(true);
#endif

	// Initializes all variables from bakkes mod settings menu
	void InitializeVariables();

	// Sets the name in the replay file and exports the replay
	string SetReplayNameAndExport(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay, string templateString);

public:
	virtual void onLoad();
	virtual void onUnload();
	
	void OnGameComplete(ServerWrapper caller, void* params, string eventName);
};