#pragma once

#include <vector>
#include <map>

using namespace std;

bool ReplaceAll(string& str, const string& from, const string& to);

vector<uint8_t> GetFileBytes(string filename);

string AppendGetParams(string baseUrl, map<string, string> getParams);

char* CopyToCharPtr(vector<uint8_t>& vector);