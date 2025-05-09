#include "shoot_3d.hpp"

entt::entity spawnBaseEnemy(entt::registry& registry, const Vector3& pos) {
	entt::entity enemy = registry.create();
    registry.emplace<Position>(enemy, pos);
    registry.emplace<Velocity>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(enemy, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(enemy, 1.0f, GREEN);
    registry.emplace<HP>(enemy, 100.0f, 100.0f);
    registry.emplace<Damage>(enemy, 50.0f);
    registry.emplace<MaxSpeed>(enemy, 10.0f);
    registry.emplace<Enemy>(enemy);
    registry.emplace<PlayerTargetable>(enemy);
    registry.emplace<AimTarget>(enemy, entt::null);
    registry.emplace<AimRotation>(enemy, Vector3{ 0, 0, 0 });

	return enemy;
}

void spawnEnemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(registry, pos);

	emplaceWeaponBasic(registry, enemy);
}

void spawnEliteEnemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(registry, pos);

    registry.emplace_or_replace<Body>(enemy, 3.0f, LIME);
    registry.emplace_or_replace<HP>(enemy, 150.0f, 150.0f);
    registry.emplace_or_replace<HPRegen>(enemy, 5.0f);
    registry.emplace_or_replace<MaxSpeed>(enemy, 15.0f);

	emplaceWeaponMachineGun(registry, enemy);

    registry.emplace_or_replace<EliteEnemy>(enemy);
}