#include "shoot_3d.hpp"

void spawn_enemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = registry.create();
    registry.emplace<Position>(enemy, pos);
    registry.emplace<Velocity>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(enemy, 1.0f, GREEN);
    registry.emplace<HP>(enemy, 100.0f, 100.0f);
    registry.emplace<Damage>(enemy, 25.0f);
    registry.emplace<Enemy>(enemy, ENEMY_SPEED, ENEMY_ATTACK_COOLDOWN, 0.0f);
}