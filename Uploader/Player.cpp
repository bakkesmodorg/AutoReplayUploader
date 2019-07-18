#include "Player.h"

Player::Player()
{
}

Player::~Player()
{
}

bool Player::WonMatch(int team0Score, int team1Score)
{
	return Team == 0 ? team0Score > team1Score : team1Score > team0Score;
}
