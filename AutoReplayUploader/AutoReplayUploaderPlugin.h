#pragma once
#pragma comment( lib, "bakkesmod.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "ISteamHTTP.h"
#include <string>

#define CALCULATED_ENDPOINT "http://127.0.0.1:8000/"
#define UPLOAD_BOUNDARY "----BakkesModFileUpload90m8924r390j34f0"
class AutoReplayUploaderPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::string userAgent;
	ISteamHTTP* steamHTTPInstance = NULL;
	std::shared_ptr<std::string> savedReplayPath = std::make_shared<std::string>("");
	std::shared_ptr<bool> uploadToCalculated = std::make_shared<bool>(false);
public:
	AutoReplayUploaderPlugin();
	virtual void onLoad();
	virtual void onUnload();
	void OnGameComplete(ServerWrapper caller, void* params, std::string eventName);
	void UploadToCalculated(std::string filename);
};