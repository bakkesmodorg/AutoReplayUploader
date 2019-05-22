#pragma once

#include <vector>
#include <map>

#include "bakkesmod/plugin/bakkesmodplugin.h"

using namespace std;

void ReplaceAll(string& str, const string& from, const string& to);

vector<uint8_t> GetFileBytes(string filename, shared_ptr<CVarManagerWrapper> cvarManager);

string AppendGetParams(string baseUrl, map<string, string> getParams);

char* CopyToCharPtr(vector<uint8_t>& vector);