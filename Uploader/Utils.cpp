#include "Utils.h"

#include <algorithm>

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
