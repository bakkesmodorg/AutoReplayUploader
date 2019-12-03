#pragma once

#include <vector>
#include <string>
#include <memory>
#include <ctime>
#include <regex>

using namespace std;

bool ReplaceAll(string& str, const string& from, const string& to);

bool RemoveChars(shared_ptr<string> str, regex matchesToRemove);

bool RemoveChars(string& str, regex matchesToRemove);