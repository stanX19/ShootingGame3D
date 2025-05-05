#include "shoot_3d.hpp"

void spawnEnemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = registry.create();
    registry.emplace<Position>(enemy, pos);
    registry.emplace<Velocity>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(enemy, 1.0f, GREEN);
    registry.emplace<HP>(enemy, 100.0f, 100.0f);
    registry.emplace<Damage>(enemy, 50.0f);
    registry.emplace<MaxSpeed>(enemy, 10.0f);
    registry.emplace<Enemy>(enemy);
	emplaceWeaponBasic(registry, enemy);
    registry.emplace<AimTarget>(enemy, entt::null);
    registry.emplace<AimRotation>(enemy, Vector3{ 0, 0, 0 });
}

void spawnEliteEnemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = registry.create();
    registry.emplace<Position>(enemy, pos);
    registry.emplace<Velocity>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(enemy, 3.0f, LIME);
    registry.emplace<HP>(enemy, 300.0f, 300.0f);
    registry.emplace<HPRegen>(enemy, 5.0f);
    registry.emplace<Damage>(enemy, 50.0f);
    registry.emplace<MaxSpeed>(enemy, 10.0f);
    registry.emplace<Enemy>(enemy);
    registry.emplace<EliteEnemy>(enemy);
	emplaceWeaponMachine_gun(registry, enemy);
    registry.emplace<AimTarget>(enemy, entt::null);
    registry.emplace<AimRotation>(enemy, Vector3{ 0, 0, 0 });
}