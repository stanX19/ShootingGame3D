#include "entities.hpp"
#include "utils.hpp"

namespace {
	Vector3 getArenaSizeVec(GameContext &context) {
		return Vector3{
			context.config.ARENA_SIZE * 1.5f,
			context.config.ARENA_SIZE * 1.5f,
			context.config.ARENA_SIZE * 1.5f
		};
	}

	t_model_id getAsteroidModel(GameContext &context) {
		std::string path = context.config.getString("units.asteroid.modelPath", "assets/Models/asteroid/asteroid_big.obj");
		return context.modelManager.loadModel(path);
	}

	Color getRandomAsteroidColor() {
		const int baseVal = 175;
		unsigned char brightness = (unsigned char)GetRandomValue(baseVal + 5, baseVal + 55);
		return Color{brightness, brightness, brightness, 255};
	}

	entt::entity spawnBaseAsteroid(GameContext &context, float rad = 100.0f) {
		float rotLerp = context.config.getFloat("units.asteroid.rotationSpeed", 0.01f);
		float damageVal = context.config.getFloat("units.asteroid.damage", 10000.0f);
		float massVal = context.config.getFloat("units.asteroid.mass", 10000.0f);
		t_model_id asteroidModel = getAsteroidModel(context);
		t_collision_mesh_id asteroidCollisionModel = context.collisionBodyManager.loadCollisionModel(context, asteroidModel);

		entt::entity asteroid = context.registry.create();
		context.registry.emplace<Rotation>(asteroid, randomRotation());
		context.registry.emplace<RotationVelocity>(asteroid, QuaternionLerp(QuaternionIdentity(), randomRotation(), rotLerp));
		context.registry.emplace<Damage>(asteroid, damageVal);
		context.registry.emplace<DisappearBound>(asteroid, getArenaSizeVec(context) * -1, getArenaSizeVec(context));
		context.registry.emplace<tag::Asteroid>(asteroid);
		context.registry.emplace<tag::Shaded>(asteroid);
		// context.registry.emplace<tag::RotationSyncModel>(asteroid);
		context.registry.emplace<CollisionBody>(asteroid, rad);
		context.registry.emplace<CollisionBodyModel>(asteroid, asteroidCollisionModel);
		context.registry.emplace<RenderBody>(asteroid, RenderBody{
			asteroidModel, getRandomAsteroidColor(), rad
		});
		context.registry.emplace<Mass>(asteroid, massVal * rad);
		return asteroid;
	}
}

void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir)
{
	float baseFactor = context.config.getFloat("units.asteroid.baseRadFactor", 0.1f);
	float rad = context.config.COMBAT_DIST * (baseFactor + GetRandomValue(0, 20) * 0.02f);
	spawnAsteroid(context, pos, dir, rad);
}

void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad)
{
	float speed = GetRandomValue(3, (int)(10 * context.config.ARENA_SIZE / 200.0f));

	for (int i = 0; i < 1; i++)
	{
		Vector3 subPos = (i == 0) ? pos : pos + randomUnitVector3() * rad;
		float subRad = (i == 0) ? rad : GetRandomValue(rad / 5, rad / 2);
		// unsigned char brightness = GetRandomValue(40, 60);
		entt::entity asteroid = spawnBaseAsteroid(context, subRad);
		context.registry.emplace<Position>(asteroid, subPos);
		context.registry.emplace<Velocity>(asteroid, Vector3Normalize(dir) * speed);
	}
}

void spawnRingAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir)
{
	float rmin = context.config.getFloat("units.asteroid.ringRadiusMin", 740.0f);
	float rmax = context.config.getFloat("units.asteroid.ringRadiusMax", 1000.0f);
	float radius = GetRandomValue((int)(rmin * 100), (int)(rmax * 100)) / 100.0f;
	Vector3 ringNormal = Vector3Normalize(randomUnitVector3());
	spawnRingAsteroid(context, pos, dir, radius, ringNormal, 10);
}

void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir, float radius, const Vector3 &ringNormal, int numAsteroids)
{
	float speed = GetRandomValue(3, (int)(10 * context.config.ARENA_SIZE / 200.0f));

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

		entt::entity asteroid = spawnBaseAsteroid(context, asteroidRadius);
		context.registry.emplace<Position>(asteroid, ringPos);
		context.registry.emplace<Velocity>(asteroid, Vector3Normalize(dir) * speed);
	}
}
