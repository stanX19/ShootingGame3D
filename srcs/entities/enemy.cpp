#include "shoot_3d.hpp"

entt::entity spawnBaseEnemy(entt::registry& registry, const Vector3& pos) {
	entt::entity enemy = registry.create();
    registry.emplace<Position>(enemy, pos);
    registry.emplace<Velocity>(enemy);
    registry.emplace<Rotation>(enemy);
    registry.emplace<CollisionBody>(enemy, 1.0f);
    registry.emplace<RenderBody>(enemy, 1.0f, GREEN);
    registry.emplace<HP>(enemy, 200.0f);
    registry.emplace<Damage>(enemy, 5000.0f);
    registry.emplace<MaxSpeed>(enemy, 10.0f);
    registry.emplace<TurnSpeed>(enemy, 2.5f);
    registry.emplace<PlayerTargetable>(enemy);
    registry.emplace<AimTarget>(enemy, entt::null);
    registry.emplace<AimDirection>(enemy);
	
    registry.emplace<tag::Enemy>(enemy);
    registry.emplace<tag::Shaded>(enemy);
	return enemy;
}

void spawnEnemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(registry, pos);

	emplaceWeaponBasic(registry, enemy);
}

void spawnEliteEnemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(registry, pos);

    registry.emplace_or_replace<CollisionBody>(enemy, 3.0f);
    registry.emplace_or_replace<RenderBody>(enemy, 3.0f, DARKGREEN);
    registry.emplace_or_replace<HP>(enemy, 300.0f);
    registry.emplace_or_replace<HPRegen>(enemy, 10.0f);
    registry.emplace_or_replace<MaxSpeed>(enemy, 5.0f);

	emplaceWeaponSniper(registry, enemy);

    registry.emplace_or_replace<tag::EliteEnemy>(enemy);
}

void spawnFastEliteEnemy(entt::registry& registry, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(registry, pos);

    registry.emplace_or_replace<CollisionBody>(enemy, 1.0f);
    registry.emplace_or_replace<RenderBody>(enemy, 1.0f, LIME);
    registry.emplace_or_replace<HP>(enemy, 160.0f);
    registry.emplace_or_replace<HPRegen>(enemy, 1.0f);
    registry.emplace_or_replace<MaxSpeed>(enemy, 40.0f);
    registry.emplace_or_replace<TurnSpeed>(enemy, 3.5f);

	emplaceWeaponMachineGun(registry, enemy);

    registry.emplace_or_replace<tag::EliteEnemy>(enemy);
}