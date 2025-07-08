#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "entities.hpp"

#define ASTEROID_COUNT 50

static Vector3 randomPosOffField() {
	return Vector3Scale(randomUnitVector3(), ARENA_SIZE * 1.5);
}

void ecs_systems::asteroidRespawn(GameContext &context)
{
	auto asteroidView = context.registry.view<tag::Asteroid>();
	int asteroidsToSpawn = ASTEROID_COUNT - (int)asteroidView.size();

	for (int i = 0; i < asteroidsToSpawn; i += 10)
	{
		Vector3 pos = randomPosOffField();
		Vector3 dir = Vector3Normalize(pos) * -2.0f + randomUnitVector3();
		if (GetRandomValue(1, 10) == 10)
			spawnRingAsteroid(context, pos, dir);
		else
			spawnAsteroid(context, pos, dir);
	}
}