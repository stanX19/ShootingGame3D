#include "shoot_3d.hpp"


void ecs_systems::hpCleanup(entt::registry& registry) {
    auto view = registry.view<HP>();
    std::vector<entt::entity> toDestroy;

    for (auto entity : view) {
        HP& hp = view.get<HP>(entity);
        if (hp.value <= 0.001f) {
            toDestroy.push_back(entity);
        }
    }

    for (auto entity : toDestroy) {
        if (!registry.valid(entity)) continue;

        if (registry.all_of<Position, Body>(entity)) {
            const Position& pos = registry.get<Position>(entity);
            const Body& body = registry.get<Body>(entity);
            spawnDebris(registry, pos.value, body.radius, body.color);
        }

        registry.destroy(entity);
    }
}