#include "shoot_3d.hpp"

void ecs_system::entity_movement(entt::registry& registry, float dt) {
    auto view = registry.view<Position, Velocity>();
    for (auto entity : view) {
        Position& position = view.get<Position>(entity);
        Velocity& velocity = view.get<Velocity>(entity);
        position.value = Vector3Add(position.value, Vector3Scale(velocity.value, dt));
    }
}