#include "systems.hpp"
#include "utils.hpp"
#include "game_utils.hpp"
#include "entities.hpp"
#include "constants.hpp"

#define ENEMY_COUNT 6

static Vector3 generateSpawnPos(const Vector3 &playerPos) {
	return game_utils::randomPosInBoxOffCombat(
		Vector3{-ARENA_SIZE * 0.5f, -ARENA_SIZE * 0.5f, ARENA_SIZE * 0.5f},
		Vector3{+ARENA_SIZE * 0.5f, +ARENA_SIZE * 0.5f, ARENA_SIZE},
		playerPos
	);
}

void ecs_systems::enemyRespawn(GameContext &context)
{
	Vector3 playerPos = {0, 0, 0};
	if (context.registry.valid(context.currentPlayer)) {
		playerPos = context.registry.get<Position>(context.currentPlayer).value;
	}

	auto eliteView = context.registry.view<tag::EliteUnit, tag::Enemy>();
	size_t eliteSize = eliteView.size_hint();
	for (size_t i = 0; i < 3 - eliteSize; i++)
	{
		if (rand() % 3 == 1)
			spawnEliteEnemy(context, generateSpawnPos(playerPos));
		else if (rand() % 2 == 1)
			spawnFastEliteEnemy(context, generateSpawnPos(playerPos));
		else
			spawnMothershipEnemy(context, generateSpawnPos(playerPos));
	}

	auto enemyView = context.registry.view<tag::Enemy>();
	int enemiesToSpawn = ENEMY_COUNT - (int)enemyView.size();

	for (int i = 0; i < enemiesToSpawn; i++)
	{
		spawnEnemy(context, generateSpawnPos(playerPos));
	}
}