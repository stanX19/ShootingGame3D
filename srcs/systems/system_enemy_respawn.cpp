#include "shoot_3d.hpp"

#define ENEMY_COUNT 5

static Vector3 randomPosInField() {
	float x = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	float z = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	float y = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	return Vector3{x, y, z};
}

void ecs_systems::enemyRespawn(entt::registry &registry)
{
	auto eliteView = registry.view<EliteEnemy>();
	if (eliteView.size() == 0)
	{
		spawnEliteEnemy(registry, randomPosInField());
	}

	auto enemyView = registry.view<Enemy>();
	int enemiesToSpawn = ENEMY_COUNT - (int)enemyView.size(); // Cast to int

	for (int i = 0; i < enemiesToSpawn; i++)
	{
		spawnEnemy(registry, randomPosInField());
	}
}