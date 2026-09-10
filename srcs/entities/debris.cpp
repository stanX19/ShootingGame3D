#include "entities.hpp"
#include "utils.hpp"

namespace {
	t_model_id getDebrisModel(ModelManager &modelManager) {
		constexpr float shards[8][3] = {
			{0.0f, 1.8f, 3.9f},
			{0.3f, 2.2f, 4.5f},
			{0.0f, 1.2f, 3.5f},
			{0.5f, 2.7f, 4.2f},
			{0.1f, 1.9f, 5.0f},
			{0.4f, 2.5f, 3.8f},
			{0.2f, 1.5f, 4.8f},
			{0.0f, 2.0943951f, 4.1887902f}
		};
		int idx = rand() % 8;
		return modelManager.createTriangle(1.0f, shards[idx][0], shards[idx][1], shards[idx][2]);
	}
}

void spawnDebris(GameContext &context, const Vector3& position, float originalRadius, Color originalColor, int count, float lifespan, Vector3 velocity) {
	for (int i = 0; i < count; ++i) {
		entt::entity debris = context.registry.create();
		t_model_id debrisModel = getDebrisModel(context.modelManager);

		float speed = 5.0f + ((float)rand() / RAND_MAX) * 5.0f;
		Vector3 debrisVel = randomUnitVector3() * speed + velocity;

		// fast = small
		float radius = originalRadius * (0.025f + 0.25f / speed);

		context.registry.emplace<physics::Position>(debris, position);
		context.registry.emplace<render::RenderBody>(debris, render::RenderBody{debrisModel, originalColor, radius, Vector3{0.0f, 0.0f, 0.0f}, randomRotation()});
		context.registry.emplace<physics::Velocity>(debris, debrisVel);
		context.registry.emplace<lifetime::Lifespan>(debris, lifespan + GetRandomValue(0, 200) / 100.0f);
		context.registry.emplace<render::tag::Shaded>(debris);
	}
}

void spawnDebris(GameContext &context, const Vector3& position, const render::RenderBody *bodyPtr, float lifespan, Vector3 velocity) {
	if (!bodyPtr)
		return;

	const float originalRadius = std::cbrt(bodyPtr->scale.x * bodyPtr->scale.y * bodyPtr->scale.z);
	const int count = static_cast<int>(std::sqrt(originalRadius)) * 25;
	spawnDebris(context, position, originalRadius, bodyPtr->color, count, lifespan, velocity);
}
