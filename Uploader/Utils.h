#pragma once

#include <string>
#include <memory>

using namespace std;

bool ReplaceAll(string& str, const string& from, const string& to);

bool SanitizeReplayNameTemplate(shared_ptr<string> replayNameTemplate, string defaultValue);

bool SanitizeExportPath(shared_ptr<string> exportPath, string defaultValue);