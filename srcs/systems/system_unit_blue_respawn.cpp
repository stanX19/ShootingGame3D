#include "systems.hpp"
#include "utils.hpp"
#include "game_utils.hpp"
#include "entities.hpp"
#include "constants.hpp"
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

static Vector3 generateSpawnPos(const Vector3 &playerPos) {
	return game_utils::randomPosInBoxOffCombat(
		Vector3{-ARENA_SIZE * 0.5f, -ARENA_SIZE * 0.5f, -ARENA_SIZE},
		Vector3{+ARENA_SIZE * 0.5f, +ARENA_SIZE * 0.5f, -ARENA_SIZE * 0.5f},
		playerPos
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
	for (int i = 0; i < 3 - eliteSize; i++)
	{
		entt::entity unit;
		if (rand() % 3 == 1)
			unit = spawnEliteUnit(context, generateSpawnPos(playerPos));
		else if (rand() % 2 == 1)
			unit = spawnFastEliteUnit(context, generateSpawnPos(playerPos));
		else
			unit = spawnMothershipUnit(context, generateSpawnPos(playerPos));
		applyTags(context, unit);
	}

	auto unitView = context.registry.view<BlueTag>();
	int enemiesToSpawn = UNIT_COUNT - (int)unitView.size();

	for (int i = 0; i < enemiesToSpawn; i++)
	{
		entt::entity unit = spawnUnit(context, generateSpawnPos(playerPos));
		applyTags(context, unit);
	}
}