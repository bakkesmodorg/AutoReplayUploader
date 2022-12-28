#pragma once

#include <vector>
#include <string>
#include <memory>
#include <ctime>
#include <regex>


bool ReplaceAll(std::string& str, const std::string& from, const std::string& to);

bool RemoveChars(std::shared_ptr<std::string> str, std::regex matchesToRemove);

bool RemoveChars(std::string& str, std::regex matchesToRemove);