#pragma once

#include <vector>
#include <string>
#include <memory>
#include <ctime>

using namespace std;

bool ReplaceAll(string& str, const string& from, const string& to);

bool RemoveChars(shared_ptr<string> str, vector<char> charsToRemove, bool changed);

bool RemoveChars(string& str, vector<char> charsToRemove, bool changed);