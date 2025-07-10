#include "entities.hpp"
#include "utils.hpp"

void spawnDebris(GameContext &context, const Vector3& position, float originalRadius, Color originalColor, int count, float lifespan, Vector3 velocity) {
	t_model_id debrisModel = context.meshManager.createBox();
	
	for (int i = 0; i < count; ++i) {
		entt::entity debris = context.registry.create();

		float speed = 5.0f + ((float)rand() / RAND_MAX) * 5.0f;
		Vector3 debrisVel = randomUnitVector3() * speed + velocity;

		// fast = small
		float radius = originalRadius * (0.05f + 0.5f / speed);

		context.registry.emplace<Position>(debris, position);
		context.registry.emplace<RenderBody>(debris, RenderBody{debrisModel, originalColor, radius, Vector3{0.0f, 0.0f, 0.0f}, randomRotation()});
		context.registry.emplace<Velocity>(debris, debrisVel);
		context.registry.emplace<Lifespan>(debris, lifespan + GetRandomValue(0, 200) / 100.0f);
		context.registry.emplace<tag::Shaded>(debris);
	}
}

