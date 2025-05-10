#include "shoot_3d.hpp"

void spawnAsteroid(entt::registry &registry, const Vector3 &pos, const Vector3 &dir)
{
	spawnAsteroid(registry, pos, dir, (rand() % 50000) / 1000.0f);
}

void spawnAsteroid(entt::registry &registry, const Vector3 &pos, const Vector3 &dir, float rad)
{
	float speed = GetRandomValue(3, 10);
	const Vector3 arenaSizeVec = Vector3{ARENA_SIZE * 2, ARENA_SIZE * 2, ARENA_SIZE * 2};

	for (int i = 0; i < 10; i++)
	{
		entt::entity asteroid = registry.create();
		Vector3 subPos = (i == 0) ? pos : pos + randomUnitVector3() * rad;
		float subRad = (i == 0) ? rad : GetRandomValue(rad / 5, rad / 2);
		// unsigned char brightness = GetRandomValue(40, 60);
		registry.emplace<Position>(asteroid, subPos);
		registry.emplace<Velocity>(asteroid, Vector3Normalize(dir) * speed);
		registry.emplace<CollisionBody>(asteroid, subRad);
		registry.emplace<RenderBody>(asteroid, subRad, (i == 0)? Color{ 40, 40, 40, 255 } : Color{ 60, 60, 60, 255 });
		registry.emplace<Damage>(asteroid, 10.0f);
		registry.emplace<DisappearBound>(asteroid, arenaSizeVec * -1, arenaSizeVec);
		registry.emplace<tag::Asteroid>(asteroid);
		registry.emplace<tag::Shaded>(asteroid);
	}
}