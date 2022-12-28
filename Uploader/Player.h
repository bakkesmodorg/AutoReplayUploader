#pragma once

#include <string>


class Player
{
public:
	std::string Name;
	unsigned long long UniqueId;
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