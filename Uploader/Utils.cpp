#include "Utils.h"

#include <iostream>
#include <ctime>
#include <sstream>

#include <algorithm>
#include <iomanip>

using namespace std;

bool ReplaceAll(string& str, const string& from, const string& to) {
	bool replaced = false;
	if (from.empty())
		return replaced;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		replaced = true;
	}
	return replaced;
}

bool SanitizeReplayNameTemplate(shared_ptr<string> replayNameTemplate, string defaultValue)
{
	bool changed = false;

	// Remove illegal characters for filename
	string illegalChars = "\\/:*?\"<>|";
	for (auto it = replayNameTemplate->begin(); it < replayNameTemplate->end(); ++it)
	{
		if (illegalChars.find(*it) != string::npos)
		{
			replayNameTemplate->erase(it);
			changed = true;
		}
	}

	// If empty use default
	if (replayNameTemplate->empty())
	{
		*replayNameTemplate = defaultValue;
		changed = true;
	}

	return changed;
}

bool SanitizeExportPath(shared_ptr<string> exportPath, string defaultValue)
{
	bool changed = false;

	// Replaces \ with /
	size_t found = exportPath->find("\\");
	if (found != string::npos)
	{
		replace(exportPath->begin(), exportPath->end(), '\\', '/');
		changed = true;
	}

	// Remove trailing slash
	if (exportPath->back() == '/')
	{
		exportPath->pop_back();
		changed = true;
	}
	
	// Remove illegal characters for a folder path
	string illegalChars = "*?\"<>|";
	for (auto it = exportPath->begin(); it < exportPath->end(); ++it)
	{
		if (illegalChars.find(*it) != string::npos)
		{
			exportPath->erase(it);
			changed = true;
		}
	}

	// If empty use default
	if (exportPath->empty())
	{
		*exportPath = defaultValue;
		changed = true;
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

string CalculateReplayName(string nameTemplate, string & mode, string & player, int teamIndex, int team0Score, int team1Score, bool & updatedNum, string num)
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
	auto won = teamIndex == 0 ? team0Score > team1Score : team1Score > team0Score;
	auto winloss = won ? string("Win") : string("Loss");
	auto wl = won ? string("W") : string("L");

	ReplaceAll(nameTemplate, "{PLAYER}", player);
	ReplaceAll(nameTemplate, "{MODE}", mode);
	ReplaceAll(nameTemplate, "{YEAR}", year);
	ReplaceAll(nameTemplate, "{MONTH}", month);
	ReplaceAll(nameTemplate, "{DAY}", day);
	ReplaceAll(nameTemplate, "{HOUR}", hour);
	ReplaceAll(nameTemplate, "{MIN}", min);
	ReplaceAll(nameTemplate, "{WINLOSS}", winloss);
	ReplaceAll(nameTemplate, "{WL}", wl);

	updatedNum = ReplaceAll(nameTemplate, "{NUM}", num);

	return nameTemplate;
}
