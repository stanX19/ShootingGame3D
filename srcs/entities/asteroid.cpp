#include "entities.hpp"
#include "utils.hpp"
#include "constants.hpp"

namespace {
	const Vector3 arenaSizeVec = Vector3{ARENA_SIZE * 4, ARENA_SIZE * 4, ARENA_SIZE * 4};

	t_model_id getAsteroidModel(GameContext &context) {
		// return context.meshManager.createSphere(64, 64);
		// return context.meshManager.loadModel("assets/Models/asteroid/asteroid_ceres.glb", Vector3{0.36f, 0.36f, 0.38f}, Vector3UnitZ, Vector3{0.5f, 0.75f, 0.5f});
		return context.meshManager.loadModel("assets/Models/asteroid/round_stone.glb");
	}

	Color getRandomAsteroidColor() {
		unsigned char brightness = (unsigned char)GetRandomValue(5, 55);
		return Color{brightness, brightness, brightness, 255};
	}

	entt::entity spawnBaseAsteroid(GameContext &context) {
		entt::entity asteroid = context.registry.create();
		context.registry.emplace<Rotation>(asteroid, randomRotation());
		context.registry.emplace<RotationVelocity>(asteroid, QuaternionLerp(QuaternionIdentity(), randomRotation(), 0.01));
		context.registry.emplace<Damage>(asteroid, 10000.0f);
		context.registry.emplace<DisappearBound>(asteroid, arenaSizeVec * -1, arenaSizeVec);
		context.registry.emplace<tag::Asteroid>(asteroid);
		context.registry.emplace<tag::Shaded>(asteroid);
		context.registry.emplace<tag::RotationSyncModel>(asteroid);
		return asteroid;
	}
}

void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir)
{
	spawnAsteroid(context, pos, dir, COMBAT_DIST * (0.1 + GetRandomValue(0, 20) * 0.02));
}

void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad)
{
	float speed = GetRandomValue(3, (int)(10 * ARENA_SIZE / 200.0f));
	t_model_id asteroidModel = getAsteroidModel(context);

	for (int i = 0; i < 1; i++)
	{
		entt::entity asteroid = spawnBaseAsteroid(context);
		Vector3 subPos = (i == 0) ? pos : pos + randomUnitVector3() * rad;
		float subRad = (i == 0) ? rad : GetRandomValue(rad / 5, rad / 2);
		// unsigned char brightness = GetRandomValue(40, 60);
		context.registry.emplace<Position>(asteroid, subPos);
		context.registry.emplace<Velocity>(asteroid, Vector3Normalize(dir) * speed);
		context.registry.emplace<CollisionBody>(asteroid, subRad);
		context.registry.emplace<RenderBody>(asteroid, RenderBody{
			asteroidModel, getRandomAsteroidColor(), subRad
		});
	}
}

void spawnRingAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir)
{
	float radius = GetRandomValue(74000, 100000) / 100.0f; // 750-1000 units
	Vector3 ringNormal = Vector3Normalize(randomUnitVector3());
	spawnRingAsteroid(context, pos, dir, radius, ringNormal, 10);
}

void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir, float radius, const Vector3 &ringNormal, int numAsteroids)
{
	float speed = GetRandomValue(3, (int)(10 * ARENA_SIZE / 200.0f));
	t_model_id asteroidModel = getAsteroidModel(context);

	Vector3 u, v;
	if (abs(ringNormal.x) < 0.9f)
		u = Vector3Normalize(Vector3CrossProduct(ringNormal, Vector3{1, 0, 0}));
	else
		u = Vector3Normalize(Vector3CrossProduct(ringNormal, Vector3{0, 1, 0}));
	v = Vector3CrossProduct(ringNormal, u);
	float asteroidRadiusAvg = (2 * PI * radius) / numAsteroids / 2 * 0.75;
	int totalAsteroids = numAsteroids + 10;

	for (int i = 0; i < totalAsteroids; i++)
	{
		float angle = ((float)i / totalAsteroids) * 2 * PI + GetRandomValue(-30, 30) * DEG2RAD;
		float asteroidRadius = asteroidRadiusAvg * (GetRandomValue(10, 100) / 100.0f);
		float radialOffset = GetRandomValue(-50, 50) / 100.0f * radius;
		Vector3 ringOffset = u * (cos(angle) * (radius + radialOffset)) + v * (sin(angle) * (radius + radialOffset));

		float verticalOffset = GetRandomValue(-50, 50) / 100.0f * radius;
		Vector3 ringPos = center + ringOffset + ringNormal * verticalOffset;

		entt::entity asteroid = spawnBaseAsteroid(context);
		context.registry.emplace<Position>(asteroid, ringPos);
		context.registry.emplace<Velocity>(asteroid, Vector3Normalize(dir) * speed);
		context.registry.emplace<CollisionBody>(asteroid, asteroidRadius);
		context.registry.emplace<RenderBody>(asteroid,
			RenderBody{asteroidModel, getRandomAsteroidColor(), asteroidRadius}
		);
	}
}
