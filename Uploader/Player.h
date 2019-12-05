#pragma once

#include <string>

using namespace std;

class Player
{
public:
	string Name;
	unsigned long long UniqueId = 0;
	int Team;

	int Score;
	int Goals;
	int Assists;
	int Saves;
	int Shots;
	int Demos;

	Player();
	~Player();

	bool WonMatch(int team0Score, int team1Score);
};