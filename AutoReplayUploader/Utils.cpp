#include "Utils.h"

#include <fstream>
#include <sstream>
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

/**
* GetReplayBytes - Loads the file specified in filename and returns the bytes
*/
vector<uint8_t> GetFileBytes(string filename)
{
	// open the file
	std::streampos fileSize;
	std::ifstream file(filename, ios::binary);

	// get its size
	file.seekg(0, ios::end);
	fileSize = file.tellg();

	if (fileSize <= 0) // in case the file does not exist for some reason
	{
		fileSize = 0;
	}

	// initialize byte vector to size of replay
	vector<uint8_t> fileData(fileSize);

	// read replay file from the beginning
	file.seekg(0, ios::beg);
	file.read((char*)&fileData[0], fileSize);
	file.close();

	return fileData;
}

string AppendGetParams(string baseUrl, map<string, string> getParams)
{
	std::stringstream urlStream;
	urlStream << baseUrl;
	if (!getParams.empty())
	{
		urlStream << "?";

		for (auto it = getParams.begin(); it != getParams.end(); it++)
		{
			if (it != getParams.begin())
				urlStream << "&";
			urlStream << (*it).first << "=" << (*it).second;
		}
	}
	return urlStream.str();
}

char* CopyToCharPtr(vector<uint8_t>& vector)
{
	char *reqData = new char[vector.size() + 1];
	for (int i = 0; i < vector.size(); i++)
		reqData[i] = vector[i];
	reqData[vector.size()] = '\0';
	return reqData;
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
