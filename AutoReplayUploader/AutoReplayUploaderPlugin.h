#pragma once
#include <string>

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"

#include "Ballchasing.h"
#include "Calculated.h"

#pragma comment( lib, "bakkesmod.lib" )

class AutoReplayUploaderPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	Ballchasing* ballchasing;
	Calculated* calculated;

	shared_ptr<bool> uploadToCalculated = std::make_shared<bool>(false);
	shared_ptr<bool> uploadToBallchasing = std::make_shared<bool>(false);
	shared_ptr<int> templateSequence = std::make_shared<int>(0);
	Logger logger;

	void InitPluginVariables();
	void SetReplayName(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay, std::string templateString);

public:
	shared_ptr<bool> showNotifications = std::make_shared<bool>(true);
	AutoReplayUploaderPlugin();

	virtual void onLoad();
	virtual void onUnload();
	
	void TestBallchasingAuth(vector<std::string> params);
	void OnGameComplete(ServerWrapper caller, void* params, std::string eventName);
};