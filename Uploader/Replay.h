#pragma once
#include "Utils.h"

#include <string>
#include "Match.h"


bool SanitizeReplayNameTemplate(std::shared_ptr<std::string> replayNameTemplate, std::string defaultValue);

std::string SanitizePlayerName(std::string playerName, std::string defaultValue);

std::string ApplyNameTemplate(std::string& nameTemplate, Match& match, int* matchIndex);

bool SanitizeExportPath(std::shared_ptr<std::string> exportPath, std::string defaultValue);

std::string CalculateReplayPath(std::string& exportDir, std::string& replayName);