#pragma once
#include <string>
#include <vector>
#include "json.hpp"
using json = nlohmann::json;

struct MMR {
	int tier;
	int division;
	int matches_played;
	float mmr;
};

struct PlayerMMRData {
	int platform_id;
	std::string id;
	MMR before;
	bool hasAfter = false;
	MMR after;
	std::string debug;

};

struct MMRData {
	std::string game;
	std::vector<PlayerMMRData> players;
};

void to_json(json& j, const MMR& p);
void to_json(json& j, const PlayerMMRData& p);
void to_json(json& j, const MMRData& p);