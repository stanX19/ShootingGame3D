#include "shoot_3d.hpp"

void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir)
{
	spawnAsteroid(context, pos, dir, (rand() % 50000) / 1000.0f);
}

void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad)
{
	float speed = GetRandomValue(3, 10);
	const Vector3 arenaSizeVec = Vector3{ARENA_SIZE * 2, ARENA_SIZE * 2, ARENA_SIZE * 2};
	t_model_id asteroidModel = context.meshManager.createSphere(64, 64);

	for (int i = 0; i < 10; i++)
	{
		entt::entity asteroid = context.registry.create();
		Vector3 subPos = (i == 0) ? pos : pos + randomUnitVector3() * rad;
		float subRad = (i == 0) ? rad : GetRandomValue(rad / 5, rad / 2);
		// unsigned char brightness = GetRandomValue(40, 60);
		context.registry.emplace<Position>(asteroid, subPos);
		context.registry.emplace<Velocity>(asteroid, Vector3Normalize(dir) * speed);
		context.registry.emplace<CollisionBody>(asteroid, subRad);
		context.registry.emplace<RenderBody>(asteroid,
			RenderBody{asteroidModel, subRad, (i == 0)? Color{ 40, 40, 40, 255 } : Color{ 60, 60, 60, 255 }}
		);
		context.registry.emplace<Damage>(asteroid, 10000.0f);
		context.registry.emplace<DisappearBound>(asteroid, arenaSizeVec * -1, arenaSizeVec);
		context.registry.emplace<tag::Asteroid>(asteroid);
		context.registry.emplace<tag::Shaded>(asteroid);
	}
}