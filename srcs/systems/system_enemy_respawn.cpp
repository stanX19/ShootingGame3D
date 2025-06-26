#include "systems.hpp"
#include "utils.hpp"
#include "entities.hpp"

#define ENEMY_COUNT 5

void ecs_systems::enemyRespawn(GameContext &context)
{
	auto eliteView = context.registry.view<tag::EliteEnemy>();
	if (eliteView.size() == 0)
	{
		if (rand() % 2)
			spawnEliteEnemy(context, randomPosInField());
		else
			spawnFastEliteEnemy(context, randomPosInField());
	}

	auto enemyView = context.registry.view<tag::Enemy>();
	int enemiesToSpawn = ENEMY_COUNT - (int)enemyView.size();

	for (int i = 0; i < enemiesToSpawn; i++)
	{
		spawnEnemy(context, randomPosInField());
	}
}