#include "Utils.h"


bool ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
	bool replaced = false;
	if (from.empty())
		return replaced;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		replaced = true;
	}
	return replaced;
}

bool RemoveChars(std::shared_ptr<std::string> str, std::regex matchesToRemove)
{
	return RemoveChars(*str, matchesToRemove);
}

bool RemoveChars(std::string& str, std::regex matchesToRemove)
{
	std::string replaced = regex_replace(str, matchesToRemove, "");
	bool changed = replaced.length() != str.length();
	str = replaced;
	return changed;
}