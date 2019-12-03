#pragma once
#include "Utils.h"

#include <string>
#include "Match.h"

using namespace std;

bool SanitizeReplayNameTemplate(shared_ptr<string> replayNameTemplate, string defaultValue);

string SanitizePlayerName(string playerName, string defaultValue);

string ApplyNameTemplate(string& nameTemplate, Match& match, int* matchIndex);

bool SanitizeExportPath(shared_ptr<string> exportPath, string defaultValue);

string CalculateReplayPath(string& exportDir, string& replayName);