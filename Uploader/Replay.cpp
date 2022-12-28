#include "Replay.h"

#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include "Utils.h"
#include <regex>

const std::regex illegalPathChars("[*?\"<>|]");
const std::regex illegalReplayChars("[\\\\/:*?\"<>|]");
const std::regex illegalNameChars("[\\\\/:*?\"<>|]");


bool SanitizeReplayNameTemplate(std::shared_ptr<std::string> replayNameTemplate, std::string defaultValue)
{
	// Remove illegal characters for filename
	bool changed = RemoveChars(replayNameTemplate, illegalReplayChars);

	// If empty use default
	if (replayNameTemplate->empty())
	{
		*replayNameTemplate = defaultValue;
		changed = true;
	}

	return changed;
}

std::string SanitizePlayerName(std::string playerName, std::string defaultValue)
{
	// Remove illegal characters for filename
	RemoveChars(playerName, illegalNameChars);

	// If empty use default
	if (playerName.empty())
	{
		playerName = defaultValue;
	}

	return playerName;
}

std::string ApplyNameTemplate(std::string& nameTemplate, Match& match, int* matchIndex)
{
	// Get date string
	auto t = time(0);
	auto now = localtime(&t);

	auto month = std::to_string(now->tm_mon + 1);
	month.insert(month.begin(), 2 - month.length(), '0');

	auto day = std::to_string(now->tm_mday);
	day.insert(day.begin(), 2 - day.length(), '0');

	auto year = std::to_string(now->tm_year + 1900);

	auto hour = std::to_string(now->tm_hour);
	hour.insert(hour.begin(), 2 - hour.length(), '0');

	auto min = std::to_string(now->tm_min);
	min.insert(min.begin(), 2 - min.length(), '0');

	// Calculate Win/Loss string
	auto won = match.PrimaryPlayer.WonMatch(match.Team0Score, match.Team1Score);
	auto winloss = won ? std::string("Win") : std::string("Loss");
	auto wl = won ? std::string("W") : std::string("L");

	std::string playerName = SanitizePlayerName(match.PrimaryPlayer.Name, "Player");

	std::string name = nameTemplate;
	ReplaceAll(name, "{MODE}", match.GameMode);
	ReplaceAll(name, "{PLAYER}", playerName);
	ReplaceAll(name, "{UNIQUEID}", std::to_string(match.PrimaryPlayer.UniqueId));
	ReplaceAll(name, "{WINLOSS}", winloss);
	ReplaceAll(name, "{WL}", wl);
	ReplaceAll(name, "{YEAR}", year);
	ReplaceAll(name, "{MONTH}", month);
	ReplaceAll(name, "{DAY}", day);
	ReplaceAll(name, "{HOUR}", hour);
	ReplaceAll(name, "{MIN}", min);

	if (ReplaceAll(name, "{NUM}", std::to_string(*matchIndex)))
	{
		*matchIndex += 1;
	}

	return name;
}

bool SanitizeExportPath(std::shared_ptr<std::string> exportPath, std::string defaultValue)
{
	// If empty use default and return OR after any below operation we return if empty
	if (exportPath->empty())
	{
		*exportPath = defaultValue;
		return true;
	}

	// Remove illegal characters for folder path
	bool changed = RemoveChars(exportPath, illegalPathChars);
	if (exportPath->empty()) { *exportPath = defaultValue;  return true; }

	// Replaces \ with /
	size_t found = exportPath->find("\\");
	if (found != std::string::npos)
	{
		replace(exportPath->begin(), exportPath->end(), '\\', '/');
		changed = true;
	}

	// Remove trailing slash
	if ((*exportPath)[exportPath->size() - 1] == '/')
	{
		exportPath->pop_back();
		changed = true;
		if (exportPath->empty()) { *exportPath = defaultValue;  return true; }
	}

	return changed;
}

std::string CalculateReplayPath(std::string& exportDir, std::string& replayName)
{
	auto t = time(nullptr);
	auto tm = *localtime(&t);

	// Use year-month-day-hour-min.replay for the replay filepath ex: 2019-05-21-14-21.replay
	std::stringstream path;
	path << exportDir << std::string("/") << replayName << " " << std::put_time(&tm, "%Y-%m-%d-%H-%M") << ".replay";

	return path.str();
}