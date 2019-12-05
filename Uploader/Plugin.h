#pragma once

#include <string>
#include <memory>

#include "Match.h"
#include "Player.h"
#include "IReplay.h"
#include "IReplayUploader.h"

using namespace std;

#define DEAULT_EXPORT_PATH "./bakkesmod/data/"
#define DEFAULT_REPLAY_NAME_TEMPLATE "{YEAR}-{MONTH}-{DAY}.{HOUR}.{MIN} {PLAYER} {MODE} {WINLOSS}"

class Plugin
{

private:
	void(*Log)(void* object, string message);
	void* Client;

	// Upload handlers
	IReplayUploader* ballchasing;
	IReplayUploader* calculated;

	bool needToUploadReplay = false;

	string SetReplayName(IReplay* replay, Match& match);
	string ExportReplay(IReplay* replay, string& replayName);

	Player BackupPlayer;

public:
	shared_ptr<bool> uploadToCalculated = make_shared<bool>(false);
	shared_ptr<bool> uploadToBallchasing = make_shared<bool>(false);

	// Replay name template variables
	shared_ptr<int> templateSequence = make_shared<int>(0);
	shared_ptr<string> replayNameTemplate = make_shared<string>(DEFAULT_REPLAY_NAME_TEMPLATE);

	// Export replay variables
	shared_ptr<bool> saveReplay = make_shared<bool>(false);
	shared_ptr<string> exportPath = make_shared<string>(DEAULT_EXPORT_PATH);

	Plugin(void(*Log)(void* object, string message), void* Client, IReplayUploader* ballchasing, IReplayUploader* calculated);

	void OnGameComplete(
		string eventName, 
		void* serverWrapper,
		IReplay*(*GetReplay)(void* serverWrapper, void(*Log)(void* object, string message), void* object),
		Match(*GetMatch)(void* serverWrapper, IReplay* replay)
	);

	void GetPlayerData(
		string eventName,
		void* gameWrapper,
		bool isOnlineGame,
		Player(*GetPlayer)(void* gameWrapper)
	);
};

