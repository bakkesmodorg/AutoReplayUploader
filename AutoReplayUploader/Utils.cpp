#include "Utils.h"
#include <fstream>
#include <sstream>

void ReplaceAll(string& str, const string& from, const string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

/**
* GetReplayBytes - Loads the file specified in filename and returns the bytes
*/
vector<uint8_t> GetFileBytes(string filename)
{
	// Open file
	ifstream replayFile(filename, ios::binary | ios::ate);

	// Determine replay size
	streamsize replayFileSize = replayFile.tellg();

	// Initialize byte vector to size of replay
	vector<uint8_t> data(replayFileSize, 0);
	data.reserve(replayFileSize);

	// Read replay file from the beginning
	replayFile.seekg(0, ios::beg);
	replayFile.read(reinterpret_cast<char*>(&data[0]), replayFileSize);
	replayFile.close();

	return data;
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
