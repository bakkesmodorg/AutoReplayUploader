#pragma once
#include <string>
#include <vector>

#include "bakkesmod/plugin/bakkesmodplugin.h"

#include "Ballchasing.h"
#include "Calculated.h"

#pragma comment( lib, "bakkesmod.lib" )

using namespace std;

class AutoReplayUploaderPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	Ballchasing* ballchasing;
	Calculated* calculated;

	shared_ptr<bool> uploadToCalculated = make_shared<bool>(false);
	shared_ptr<bool> uploadToBallchasing = make_shared<bool>(false);
	shared_ptr<bool> saveReplay = make_shared<bool>(false);
	shared_ptr<int> templateSequence = make_shared<int>(0);

	void InitPluginVariables();
	string SetReplayNameAndExport(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay, string templateString);

public:
	virtual void onLoad();
	virtual void onUnload();
	
	void TestBallchasingAuth(vector<string> params);
	void OnGameComplete(ServerWrapper caller, void* params, string eventName);

	shared_ptr<bool> showNotifications = make_shared<bool>(true);
};