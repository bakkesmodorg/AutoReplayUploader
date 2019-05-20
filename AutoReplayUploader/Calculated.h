#pragma once

#include <iostream>
#include <vector>

using namespace std;

class Calculated
{
private:
	string UserAgent;
	string UploadBoundary;
public:
	Calculated(string userAgent, string uploadBoundary);
	bool UploadReplay(string replayPath);
	~Calculated();
};

