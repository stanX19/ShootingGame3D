#include "shoot_3d.hpp"

#define ENEMY_COUNT 5

void ecs_systems::enemyRespawn(entt::registry &registry)
{
	auto eliteView = registry.view<tag::EliteEnemy>();
	if (eliteView.size() == 0)
	{
		if (rand() % 2)
			spawnEliteEnemy(registry, randomPosInField());
		else
			spawnFastEliteEnemy(registry, randomPosInField());
	}

	auto enemyView = registry.view<tag::Enemy>();
	int enemiesToSpawn = ENEMY_COUNT - (int)enemyView.size();

	for (int i = 0; i < enemiesToSpawn; i++)
	{
		spawnEnemy(registry, randomPosInField());
	}
}