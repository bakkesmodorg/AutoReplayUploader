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


bool SanitizeReplayNameTemplate(shared_ptr<string> replayNameTemplate, string defaultValue)
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

string SanitizePlayerName(string playerName, string defaultValue)
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

string ApplyNameTemplate(string& nameTemplate, Match& match, int* matchIndex)
{
	// Get date string
	auto t = time(0);
	auto now = localtime(&t);

	auto month = to_string(now->tm_mon + 1);
	month.insert(month.begin(), 2 - month.length(), '0');

	auto day = to_string(now->tm_mday);
	day.insert(day.begin(), 2 - day.length(), '0');

	auto year = to_string(now->tm_year + 1900);

	auto hour = to_string(now->tm_hour);
	hour.insert(hour.begin(), 2 - hour.length(), '0');

	auto min = to_string(now->tm_min);
	min.insert(min.begin(), 2 - min.length(), '0');

	// Calculate Win/Loss string
	auto won = match.PrimaryPlayer.WonMatch(match.Team0Score, match.Team1Score);
	auto winloss = won ? string("Win") : string("Loss");
	auto wl = won ? string("W") : string("L");

	string playerName = SanitizePlayerName(match.PrimaryPlayer.Name, "Player");

	string name = nameTemplate;
	ReplaceAll(name, "{MODE}", match.GameMode);
	ReplaceAll(name, "{PLAYER}", playerName);
	ReplaceAll(name, "{UNIQUEID}", to_string(match.PrimaryPlayer.UniqueId));
	ReplaceAll(name, "{WINLOSS}", winloss);
	ReplaceAll(name, "{WL}", wl);
	ReplaceAll(name, "{YEAR}", year);
	ReplaceAll(name, "{MONTH}", month);
	ReplaceAll(name, "{DAY}", day);
	ReplaceAll(name, "{HOUR}", hour);
	ReplaceAll(name, "{MIN}", min);

	if (ReplaceAll(name, "{NUM}", to_string(*matchIndex)))
	{
		*matchIndex += 1;
	}

	return name;
}

bool SanitizeExportPath(shared_ptr<string> exportPath, string defaultValue)
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
	if (found != string::npos)
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

string CalculateReplayPath(string& exportDir, string& replayName)
{
	auto t = time(nullptr);
	auto tm = *localtime(&t);

	// Use year-month-day-hour-min.replay for the replay filepath ex: 2019-05-21-14-21.replay
	stringstream path;
	path << exportDir << string("/") << replayName << " " << put_time(&tm, "%Y-%m-%d-%H-%M") << ".replay";

	return path.str();
}