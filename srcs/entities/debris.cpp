#include "entities.hpp"
#include "utils.hpp"

void spawnDebris(GameContext &context, const Vector3& position, float originalRadius, Color originalColor, int count, float lifespan) {
    t_model_id debrisModel = context.meshManager.createBox();
	
	for (int i = 0; i < count; ++i) {
        entt::entity debris = context.registry.create();

        Vector3 dir = {
            (float)rand() / RAND_MAX * 2.0f - 1.0f,
            (float)rand() / RAND_MAX * 2.0f - 1.0f,
            (float)rand() / RAND_MAX * 2.0f - 1.0f
        };
        dir = Vector3Normalize(dir);

        float speed = 5.0f + ((float)rand() / RAND_MAX) * 5.0f;
        Vector3 velocity = dir * speed;

        // fast = small
        float radius = originalRadius * (0.05f + 0.5f / speed);

        context.registry.emplace<Position>(debris, position);
        context.registry.emplace<RenderBody>(debris, RenderBody{debrisModel, radius, originalColor, Vector3{0.0f, 0.0f, 0.0f}, randomRotation()});
        context.registry.emplace<Velocity>(debris, velocity);
        context.registry.emplace<Lifetime>(debris, lifespan);
    	context.registry.emplace<tag::Shaded>(debris);
    }
}

