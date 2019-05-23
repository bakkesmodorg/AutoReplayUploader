#pragma once

#include <string>
#include <memory>
#include <ctime>

using namespace std;

bool ReplaceAll(string& str, const string& from, const string& to);

bool SanitizeReplayNameTemplate(shared_ptr<string> replayNameTemplate, string defaultValue);

bool SanitizeExportPath(shared_ptr<string> exportPath, string defaultValue);

string CalculateReplayName(string nameTemplate, string& mode, string& player, int teamIndex, int team0Score, int team1Score, bool& updatedNum, string num);

string CalculateReplayPath(string& exportDir, string& replayName);