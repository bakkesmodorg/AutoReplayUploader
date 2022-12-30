#include "Player.h"

Player::Player()
{
}

Player::~Player()
{
}

bool Player::WonMatch(int winningTeam)
{
	return Team == winningTeam;
}
