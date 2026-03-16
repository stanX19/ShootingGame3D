#include "systems.hpp"
#include "utils.hpp"
#include "game_utils.hpp"
#include "entities.hpp"
#include "components/factions.hpp"

namespace {
	struct BlueTag {};

	void applyTags(GameContext &context, entt::entity entity) {
		context.registry.emplace<BlueTag>(entity);
		context.registry.emplace_or_replace<faction::Faction>(entity, faction::FAC_BLUE);
		RenderBody *body = context.registry.try_get<RenderBody>(entity);
		if (body)
			body->color = SKYBLUE;
	}
}

static Vector3 generateSpawnPos(GameContext const &context, Vector3 const &playerPos) {
	float arenaSize = context.config.ARENA_SIZE;
	const float box = 0.2f;
	const float dist = 0.2f;

	return game_utils::randomPosInBoxOffCombat(
		Vector3{-arenaSize * box, -arenaSize * box, -arenaSize * (1.0f - dist)},
		Vector3{+arenaSize * box, +arenaSize * box, -arenaSize * (1.0f - box - dist)},
		playerPos,
		context.config.COMBAT_DIST
	);
}

void ecs_systems::blueUnitRespawn(GameContext &context, [[maybe_unused]] float dt)
{
	Vector3 playerPos = {0, 0, 0};
	if (context.registry.valid(context.currentPlayer)) {
		playerPos = context.registry.get<Position>(context.currentPlayer).value;
	}

	auto eliteView = context.registry.view<tag::EliteUnit, BlueTag>();
	int eliteSize = (int)eliteView.size_hint();
	int elitesToSpawn = context.config.UNIT_COUNT / 2 - eliteSize;
	for (int i = 0; i < elitesToSpawn; i++)
	{
		entt::entity unit;
		if (rand() % 3 == 1)
			unit = spawnEliteUnit(context, generateSpawnPos(context, playerPos));
		else if (rand() % 2 == 1)
			unit = spawnFastEliteUnit(context, generateSpawnPos(context, playerPos));
		else
			unit = spawnMothershipUnit(context, generateSpawnPos(context, playerPos));
		applyTags(context, unit);
	}

	auto unitView = context.registry.view<BlueTag>();
	int enemiesToSpawn = context.config.UNIT_COUNT - (int)unitView.size();

	for (int i = 0; i < enemiesToSpawn; i++)
	{
		entt::entity unit = spawnUnit(context, generateSpawnPos(context, playerPos));
		applyTags(context, unit);
	}
}