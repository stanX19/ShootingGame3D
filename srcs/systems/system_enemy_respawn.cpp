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
	auto playerView = context.registry.view<tag::Player, Position>();
	Vector3 playerPos = {0, 0, 0};
	if (playerView.begin() != playerView.end()) {
		playerPos = playerView.get<Position>(*playerView.begin()).value;
	}

	auto eliteView = context.registry.view<tag::EliteEnemy>();
	for (size_t i = 0; i < 3 - eliteView.size(); i++)
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