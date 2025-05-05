#include "shoot_3d.hpp"

void ecs_systems::enemyMoveControl(entt::registry& registry, float dt) {
    (void)dt;

    auto playerView = registry.view<Player, Position>();
    if (playerView.begin() == playerView.end()) return;

    // Get the first player's position (assuming single-player)
    Position playerPos = playerView.get<Position>(*playerView.begin());

    auto enemyView = registry.view<Enemy, Position, Rotation, Velocity, MaxSpeed>();

    for (auto entity : enemyView) {
		Position& position = enemyView.get<Position>(entity);
        Rotation& rotation = enemyView.get<Rotation>(entity);
        Velocity& velocity = enemyView.get<Velocity>(entity);
		MaxSpeed &maxSpeed = enemyView.get<MaxSpeed>(entity);

        // Calculate direction to player
        Vector3 toPlayer = Vector3Subtract(playerPos.value, position.value);
        float dist = Vector3Length(toPlayer);
        Vector3 direction = Vector3Normalize(toPlayer);

        // Use utility to convert direction to rotation
        rotation.value = vector3ToRotation(direction);

        // Move toward player if not too close
        if (dist > 10.0f) {
            velocity.value = Vector3Scale(direction, maxSpeed.value);
        } else {
            velocity.value = { 0, 0, 0 };
        }
    }
}

