#pragma once
#include <string>

#include "bakkesmod/plugin/bakkesmodplugin.h"

#include "Plugin.h"
#include "Ballchasing.h"
#include "Calculated.h"

#pragma comment( lib, "bakkesmod.lib" )

using namespace std;

class AutoReplayUploaderPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	// Plugin object which houses as much of the functionality as possible
	Plugin* plugin;

	// Upload handlers
	Ballchasing* ballchasing;
	Calculated* calculated;

	// Initializes all variables from bakkes mod settings menu
	void InitializeVariables();

public:
	virtual void onLoad();
	virtual void onUnload();
	
	void GetPlayerData(ServerWrapper caller, void* params, string eventName);
	void OnGameComplete(ServerWrapper caller, void* params, string eventName);

#ifdef TOAST
	shared_ptr<bool> showNotifications = make_shared<bool>(true);
#endif
};