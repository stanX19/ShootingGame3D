#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "entities.hpp"

#define ASTEROID_COUNT 10

static Vector3 randomPosOffCombat(Vector3 playerPos) {
	Vector3 pos;
	do
		pos = randomUnitVector3() * (COMBAT_DIST + 1500);
	while (Vector3Distance(pos, playerPos) < COMBAT_DIST + 1000);
	return pos;
}

void ecs_systems::asteroidRespawn(GameContext &context)
{
	auto asteroidView = context.registry.view<tag::Asteroid>();
	int asteroidsToSpawn = ASTEROID_COUNT - (int)asteroidView.size();

	Vector3 playerPos = {0, 0, 0};
	if (context.registry.valid(context.currentPlayer))
		playerPos = context.registry.get<Position>(context.currentPlayer).value;

	for (int i = 0; i < asteroidsToSpawn; i += 10)
	{
		Vector3 pos = randomPosOffCombat(playerPos);
		Vector3 dir = Vector3Normalize(pos) * -2.0f + randomUnitVector3();
		if (GetRandomValue(1, 10) == 10)
			spawnRingAsteroid(context, pos, dir);
		else
			spawnAsteroid(context, pos, dir);
	}
}