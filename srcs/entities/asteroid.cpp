#include "entities.hpp"
#include "utils.hpp"
#include "constants.hpp"

namespace {
	const Vector3 arenaSizeVec = Vector3{ARENA_SIZE * 4, ARENA_SIZE * 4, ARENA_SIZE * 4};
}

void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir)
{
	spawnAsteroid(context, pos, dir, COMBAT_DIST * (0.08 + GetRandomValue(0, 20) * 0.01));
}

void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad)
{
	float speed = GetRandomValue(3, (int)(10 * ARENA_SIZE / 200.0f));
	t_model_id asteroidModel = context.meshManager.createSphere(64, 64);

	for (int i = 0; i < 5; i++)
	{
		entt::entity asteroid = context.registry.create();
		Vector3 subPos = (i == 0) ? pos : pos + randomUnitVector3() * rad;
		float subRad = (i == 0) ? rad : GetRandomValue(rad / 5, rad / 2);
		// unsigned char brightness = GetRandomValue(40, 60);
		context.registry.emplace<Position>(asteroid, subPos);
		context.registry.emplace<Velocity>(asteroid, Vector3Normalize(dir) * speed);
		context.registry.emplace<CollisionBody>(asteroid, subRad);
		context.registry.emplace<RenderBody>(asteroid,
			RenderBody{asteroidModel, (i == 0)? Color{ 40, 40, 40, 255 } : Color{ 60, 60, 60, 255 }, subRad}
		);
		context.registry.emplace<Damage>(asteroid, 500.0f);
		context.registry.emplace<DisappearBound>(asteroid, arenaSizeVec * -1, arenaSizeVec);
		context.registry.emplace<tag::Asteroid>(asteroid);
		context.registry.emplace<tag::Shaded>(asteroid);
	}
}

void spawnRingAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir)
{
	float radius = GetRandomValue(50000, 75000) / 100.0f; // 500-750 units
	Vector3 ringNormal = Vector3Normalize(randomUnitVector3());
	spawnRingAsteroid(context, pos, dir, radius, ringNormal, 10);
}

void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir, float radius, const Vector3 &ringNormal, int numAsteroids)
{
	float speed = GetRandomValue(3, (int)(10 * ARENA_SIZE / 200.0f));
	t_model_id asteroidModel = context.meshManager.createSphere(64, 64);
	
	Vector3 u, v;
	if (abs(ringNormal.x) < 0.9f)
		u = Vector3Normalize(Vector3CrossProduct(ringNormal, Vector3{1, 0, 0}));
	else
		u = Vector3Normalize(Vector3CrossProduct(ringNormal, Vector3{0, 1, 0}));
	v = Vector3CrossProduct(ringNormal, u);
	
	float asteroidRadiusAvg = (2 * PI * radius) / numAsteroids / 2;  // circum / numAsteroid
	for (int i = 0; i < numAsteroids; i++)
	{
		float angle = (float)i / numAsteroids * 2 * PI + GetRandomValue(-10, 10) * DEG2RAD;
		float asteroidRadius = GetRandomValue((int)(asteroidRadiusAvg * 70), (int)(asteroidRadiusAvg * 99)) / 100.0f;
		Vector3 ringOffset = u * (cos(angle) * radius) + v * (sin(angle) * radius);
		Vector3 ringPos = center + ringOffset + ringNormal * (asteroidRadius / 5.0f);
		unsigned char brightness = (unsigned char)GetRandomValue(40, 70);
		entt::entity asteroid = context.registry.create();
		context.registry.emplace<Position>(asteroid, ringPos);
		context.registry.emplace<Velocity>(asteroid, Vector3Normalize(dir) * speed);
		context.registry.emplace<CollisionBody>(asteroid, asteroidRadius);
		context.registry.emplace<RenderBody>(asteroid,
			RenderBody{asteroidModel, Color{brightness, brightness, brightness, 255}, asteroidRadius}
		);
		context.registry.emplace<Damage>(asteroid, 10000.0f);
		context.registry.emplace<DisappearBound>(asteroid, arenaSizeVec * -1, arenaSizeVec);
		context.registry.emplace<tag::Asteroid>(asteroid);
		context.registry.emplace<tag::Shaded>(asteroid);
	}
}

