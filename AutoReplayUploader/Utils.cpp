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