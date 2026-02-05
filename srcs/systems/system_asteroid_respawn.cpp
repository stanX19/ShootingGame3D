#include "systems.hpp"
#include "utils.hpp"
#include "game_utils.hpp"
#include "entities.hpp"

#define ASTEROID_COUNT 10

void ecs_systems::asteroidRespawn(GameContext &context, [[maybe_unused]] float dt)
{
	auto asteroidView = context.registry.view<tag::Asteroid>();
	int asteroidsToSpawn = ASTEROID_COUNT - (int)asteroidView.size();

	Vector3 playerPos = {0, 0, 0};
	if (context.registry.valid(context.currentPlayer))
		playerPos = context.registry.get<Position>(context.currentPlayer).value;

	for (int i = 0; i < asteroidsToSpawn; i += 10)
	{
		Vector3 pos = game_utils::randomPosOffCombat(playerPos, context.config.COMBAT_DIST);
		Vector3 dir = Vector3Normalize(pos) * -2.0f + randomUnitVector3();
		if (GetRandomValue(1, 10) == 10)
			spawnRingAsteroid(context, pos, dir);
		else
			spawnAsteroid(context, pos, dir);
	}
}