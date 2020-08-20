#pragma once

#include "Player.h"

#include <string>
#include <vector>

using namespace std;

class Match
{
public:

	string GameMode;
	Player PrimaryPlayer;
	vector<Player> Players;

	int Team0Score;
	int Team1Score;
	int WinningTeam;
	Match();
	~Match();
};