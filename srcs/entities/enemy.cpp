#include "shoot_3d.hpp"

void spawn_enemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = registry.create();
    registry.emplace<Position>(enemy, pos);
    registry.emplace<Velocity>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(enemy, 1.0f, GREEN);
    registry.emplace<HP>(enemy, 100.0f, 100.0f);
    registry.emplace<Damage>(enemy, 50.0f);
    registry.emplace<MaxSpeed>(enemy, 10.0f);
    registry.emplace<Enemy>(enemy);
	emplace_weapon_basic(registry, enemy);
}