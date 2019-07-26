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

bool RemoveChars(shared_ptr<string> str, vector<char> charsToRemove, bool changed)
{
	// Remove illegal characters for a folder path
	string output;
	output.reserve(str->size());
	for (size_t i = 0; i < str->size(); ++i)
	{
		char c = (*str)[i];
		bool found = false;
		for (size_t j = 0; j < charsToRemove.size(); j++)
		{
			if (c == charsToRemove[j])
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			output += c;
		}
		else
		{
			changed = true;
		}
	}
	*str = output;
	return changed;
}