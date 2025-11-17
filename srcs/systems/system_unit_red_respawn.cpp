#include "systems.hpp"
#include "utils.hpp"
#include "game_utils.hpp"
#include "entities.hpp"
#include "constants.hpp"
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

static Vector3 generateSpawnPos(const Vector3 &playerPos) {
	return game_utils::randomPosInBoxOffCombat(
		Vector3{-ARENA_SIZE * 0.5f, -ARENA_SIZE * 0.5f, ARENA_SIZE * 0.5f},
		Vector3{+ARENA_SIZE * 0.5f, +ARENA_SIZE * 0.5f, ARENA_SIZE},
		playerPos
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
	// std::cout << "Current Elite Count: " << eliteSize << std::endl;

	// Spawn elites until we have 3
	for (int i = 0; i < 3 - eliteSize; i++)
	{
		entt::entity unit;
		if (rand() % 3 == 1) {
			unit = spawnEliteUnit(context, generateSpawnPos(playerPos));
			// std::cout << "Spawned EliteUnit entity " << (int)unit << std::endl;
		}
		else if (rand() % 2 == 1) {
			unit = spawnFastEliteUnit(context, generateSpawnPos(playerPos));
			// std::cout << "Spawned FastEliteUnit entity " << (int)unit << std::endl;
		}
		else {
			unit = spawnMothershipUnit(context, generateSpawnPos(playerPos));
			// std::cout << "Spawned MothershipUnit entity " << (int)unit << std::endl;
		}
		applyTags(context, unit);
	}

	// Count total red units (all kinds)
	auto unitView = context.registry.view<RedTag>();
	int redCount = (int)unitView.size();
	int enemiesToSpawn = UNIT_COUNT - redCount;

	// std::cout << "Total Red Units (with tag): " << redCount << std::endl;
	// std::cout << "EnemiesToSpawn: " << enemiesToSpawn << " (Target = " << UNIT_COUNT << ")" << std::endl;

	// Spawn regular units
	for (int i = 0; i < enemiesToSpawn; i++)
	{
		entt::entity unit = spawnUnit(context, generateSpawnPos(playerPos));
		// std::cout << "Spawned RegularUnit entity " << (int)unit << std::endl;
		applyTags(context, unit);
	}
}
