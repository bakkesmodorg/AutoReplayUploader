#pragma once

#include <vector>
#include <map>

using namespace std;

void ReplaceAll(string& str, const string& from, const string& to);

vector<uint8_t> GetFileBytes(string filename);

string AppendGetParams(string baseUrl, map<string, string> getParams);