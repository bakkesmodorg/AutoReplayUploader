#include "MMRData.h"

void to_json(json& j, const MMR& p)
{
	j = { {"tier", p.tier}, {"division", p.division},{"matches_played", p.matches_played},  {"mmr", p.mmr} };
}

void to_json(json& j, const PlayerMMRData& p)
{
	j = { {"platform_id", p.platform_id}, {"id", p.id}, {"before", p.before}, {"debug", p.debug} };
	if (p.hasAfter) {
		j["after"] = p.after;
	}
}

void to_json(json& j, const MMRData& p)
{
	j = { {"game", p.game}, {"players", p.players} };
}
