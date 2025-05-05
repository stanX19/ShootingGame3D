#include "shoot_3d.hpp"

void spawnDebris(entt::registry& registry, const Vector3& position, float originalRadius, Color originalColor, int count, float lifespan) {
    for (int i = 0; i < count; ++i) {
        entt::entity debris = registry.create();

        Vector3 dir = {
            (float)rand() / RAND_MAX * 2.0f - 1.0f,
            (float)rand() / RAND_MAX * 2.0f - 1.0f,
            (float)rand() / RAND_MAX * 2.0f - 1.0f
        };
        dir = Vector3Normalize(dir);

        float speed = 5.0f + ((float)rand() / RAND_MAX) * 5.0f;
        Vector3 velocity = Vector3Scale(dir, speed);

        // Radius shrinks with speed but is proportional to original size
        float radius = originalRadius * (0.05f + 0.5f / speed); // clamp-like scaling

        registry.emplace<Position>(debris, position);
        registry.emplace<Body>(debris, radius, originalColor);
        registry.emplace<Velocity>(debris, velocity);
        registry.emplace<Lifetime>(debris, lifespan);
    }
}

