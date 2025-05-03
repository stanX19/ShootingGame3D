#include "shoot_3d.hpp"

void ecs_system::enemy_update(entt::registry& registry, float dt) {
    auto playerView = registry.view<Player, Position>();
    if (playerView.begin() == playerView.end()) return;

    Position playerPos = playerView.get<Position>(playerView.front());
    auto enemyView = registry.view<Enemy, Position, Rotation, Velocity, Body>();

    for (auto entity : enemyView) {
        Enemy& enemy = enemyView.get<Enemy>(entity);
        Position& position = enemyView.get<Position>(entity);
        Rotation& rotation = enemyView.get<Rotation>(entity);
        Velocity& velocity = enemyView.get<Velocity>(entity);
        Body& body = enemyView.get<Body>(entity);

        // Direction to player
        Vector3 toPlayer = Vector3Subtract(playerPos.value, position.value);
        float dist = Vector3Length(toPlayer);
        Vector3 direction = Vector3Normalize(toPlayer);

        // Update rotation to face player - fixed to work with our direction system
        rotation.value.y = atan2f(direction.x, direction.z);

        // Calculate pitch angle with clamping
        float pitchAngle = atan2f(-direction.y, sqrtf(direction.x * direction.x + direction.z * direction.z));
        rotation.value.x = Clamp(pitchAngle, -1.5f, 1.5f);

        // Move toward player if not too close
        if (dist > 10.0f) {
            velocity.value = Vector3Scale(direction, enemy.moveSpeed);
        }
        else {
            velocity.value = { 0, 0, 0 };
        }

        // Shooting behavior - improved timing
        enemy.timeSinceLastAttack += dt;
        if (enemy.timeSinceLastAttack >= enemy.attackCooldown) {
            Vector3 spawnPos = Vector3Add(position.value, Vector3Scale(direction, body.radius + 1));
            spawn_bullet(registry, spawnPos, direction, ENEMY_BULLET_DAMAGE, RED);
            enemy.timeSinceLastAttack = 0.0f;
        }
    }
}