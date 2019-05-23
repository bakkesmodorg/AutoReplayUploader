#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

using namespace std;

bool ReplaceAll(string& str, const string& from, const string& to);

vector<uint8_t> GetFileBytes(string filename);

string AppendGetParams(string baseUrl, map<string, string> getParams);

char* CopyToCharPtr(vector<uint8_t>& vector);

bool SanitizeReplayNameTemplate(shared_ptr<string> replayNameTemplate, string defaultValue);

bool SanitizeExportPath(shared_ptr<string> exportPath, string defaultValue);