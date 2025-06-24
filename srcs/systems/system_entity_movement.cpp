#include "shoot_3d.hpp"

void ecs_systems::entityMovement(GameContext &context, float dt) {
    auto view = context.registry.view<Position, Velocity>();
    for (auto entity : view) {
        Position& position = view.get<Position>(entity);
        Velocity& velocity = view.get<Velocity>(entity);
        position.value = position.value + velocity.value * dt;
    }
}