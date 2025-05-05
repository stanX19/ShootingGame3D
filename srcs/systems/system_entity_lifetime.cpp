#include "shoot_3d.hpp"

void ecs_systems::entityLifetime(entt::registry& registry, float dt) {
    auto view = registry.view<Lifetime>();
    std::vector<entt::entity> toDestroy;

    for (auto entity : view) {
        Lifetime& lifetime = view.get<Lifetime>(entity);
        lifetime.value -= dt;

        if (lifetime.value <= 0) {
            toDestroy.push_back(entity);
        }
    }

    for (auto entity : toDestroy) {
        if (registry.valid(entity)) {
            registry.destroy(entity);
        }
    }
}