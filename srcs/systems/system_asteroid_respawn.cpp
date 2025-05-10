#include "shoot_3d.hpp"

#define ASTEROID_COUNT 50

static Vector3 randomPosOffField() {
	return Vector3Scale(randomUnitVector3(), ARENA_SIZE * 1.5);
}

void ecs_systems::asteroidRespawn(entt::registry &registry)
{
	auto asteroidView = registry.view<tag::Asteroid>();
	int asteroidsToSpawn = ASTEROID_COUNT - (int)asteroidView.size();

	for (int i = 0; i < asteroidsToSpawn; i += 10)
	{
		Vector3 pos = randomPosOffField();
		Vector3 dir = Vector3Normalize(pos) * -2.0f + randomUnitVector3();
		spawnAsteroid(registry, pos, dir);
	}
}