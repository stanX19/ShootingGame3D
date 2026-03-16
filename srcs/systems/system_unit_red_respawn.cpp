#include "systems.hpp"
#include "utils.hpp"
#include "game_utils.hpp"
#include "entities.hpp"
#include "components/factions.hpp"
#include <iostream>

namespace {
	struct RedTag {};

	void applyTags(GameContext &context, entt::entity entity) {
		context.registry.emplace<RedTag>(entity);
		context.registry.emplace_or_replace<faction::Faction>(entity, faction::FAC_RED);
		RenderBody *body = context.registry.try_get<RenderBody>(entity);
		if (body)
			body->color = YELLOW;

		// std::cout << "[applyTags] Applied RedTag + Faction::FAC_RED to entity " 
		// 		  << (int)entity << std::endl;
	}
}

static Vector3 generateSpawnPos(GameContext const &context, Vector3 const &playerPos) {
	float arenaSize = context.config.ARENA_SIZE;
	const float box = 0.2f;
	const float dist = 0.2f;

	return game_utils::randomPosInBoxOffCombat(
		Vector3{-arenaSize * box, -arenaSize * box, arenaSize * (1.0f - box - dist)},
		Vector3{+arenaSize * box, +arenaSize * box, arenaSize * (1.0f - dist)},
		playerPos,
		context.config.COMBAT_DIST
	);
}

void ecs_systems::redUnitRespawn(GameContext &context, [[maybe_unused]] float dt)
{
	Vector3 playerPos = {0, 0, 0};
	if (context.registry.valid(context.currentPlayer)) {
		playerPos = context.registry.get<Position>(context.currentPlayer).value;
	}

	// std::cout << "\n=== [redUnitRespawn Tick] ===" << std::endl;
	// std::cout << "PlayerPos: (" << playerPos.x << ", " 
								//   << playerPos.y << ", " 
								//   << playerPos.z << ")" << std::endl;

	// Count elites
	auto eliteView = context.registry.view<tag::EliteUnit, RedTag>();
	int eliteSize = (int)eliteView.size_hint();
	int elitesToSpawn = context.config.UNIT_COUNT / 2 - eliteSize;
	// std::cout << "Current Elite Count: " << eliteSize << std::endl;

	// Spawn elites until we have 3
	for (int i = 0; i < elitesToSpawn; i++)
	{
		entt::entity unit;
		if (rand() % 3 == 1) {
			unit = spawnEliteUnit(context, generateSpawnPos(context, playerPos));
			// std::cout << "Spawned EliteUnit entity " << (int)unit << std::endl;
		}
		else if (rand() % 2 == 1) {
			unit = spawnFastEliteUnit(context, generateSpawnPos(context, playerPos));
			// std::cout << "Spawned FastEliteUnit entity " << (int)unit << std::endl;
		}
		else {
			unit = spawnMothershipUnit(context, generateSpawnPos(context, playerPos));
			// std::cout << "Spawned MothershipUnit entity " << (int)unit << std::endl;
		}
		applyTags(context, unit);
	}

	// Count total red units (all kinds)
	auto unitView = context.registry.view<RedTag>();
	int redCount = (int)unitView.size();
	int enemiesToSpawn = context.config.UNIT_COUNT - redCount;

	// std::cout << "Total Red Units (with tag): " << redCount << std::endl;
	// std::cout << "EnemiesToSpawn: " << enemiesToSpawn << " (Target = " << UNIT_COUNT << ")" << std::endl;

	// Spawn regular units
	for (int i = 0; i < enemiesToSpawn; i++)
	{
		entt::entity unit = spawnUnit(context, generateSpawnPos(context, playerPos));
		// std::cout << "Spawned RegularUnit entity " << (int)unit << std::endl;
		applyTags(context, unit);
	}
}
