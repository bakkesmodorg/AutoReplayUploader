#include "Utils.h"

using namespace std;

bool ReplaceAll(string& str, const string& from, const string& to) {
	bool replaced = false;
	if (from.empty())
		return replaced;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		replaced = true;
	}
	return replaced;
}

bool RemoveChars(shared_ptr<string> str, regex matchesToRemove)
{
	return RemoveChars(*str, matchesToRemove);
}

bool RemoveChars(string& str, regex matchesToRemove)
{
	string replaced = regex_replace(str, matchesToRemove, "");
	bool changed = replaced.length() != str.length();
	str = replaced;
	return changed;
}