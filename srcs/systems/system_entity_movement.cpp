#include "shoot_3d.hpp"

void ecs_systems::entityMovement(entt::registry& registry, float dt) {
    auto view = registry.view<Position, Velocity>();
    for (auto entity : view) {
        Position& position = view.get<Position>(entity);
        Velocity& velocity = view.get<Velocity>(entity);
        position.value = position.value + velocity.value * dt;
    }
}