#pragma once

#include "Player.h"

#include <string>
#include <vector>


class Match
{
public:

	std::string GameMode;
	Player PrimaryPlayer;
	std::vector<Player> Players;

	int Team0Score;
	int Team1Score;
	int WinningTeam;
	Match();
	~Match();
};