#include "shoot_3d.hpp"

static Vector3 getEntityAimNormalized(entt::registry& registry, entt::entity e) {
	if (registry.all_of<AimRotation>(e)) {
		return GetForwardVector(registry.get<AimRotation>(e).value);
	}
	if (registry.all_of<Rotation>(e)) {
		return GetForwardVector(registry.get<Rotation>(e).value);
	}
	return {0, 0, 1}; // Default forward
}

void ecs_systems::bulletWeaponShoot(entt::registry& registry, float dt) {
    auto view = registry.view<BulletWeapon, Position>();

    for (auto entity : view) {
        auto& weapon = view.get<BulletWeapon>(entity);
        auto& pos = view.get<Position>(entity);

        weapon.timeSinceLastShot += dt;

        if (!weapon.firing || weapon.timeSinceLastShot < weapon.shootCooldown)
            continue;

        // Ammo check (optional)
        if (registry.any_of<Ammo>(entity)) {
            auto& ammo = registry.get<Ammo>(entity);
            if (ammo.value < 1.0f)
                continue;

            ammo.value = std::max(ammo.value - 1.0f, 0.0f);
        }

        // Determine aim direction
        Vector3 dir = getEntityAimNormalized(registry, entity);

        // Spawn bullet
        entt::entity bullet = registry.create();

		float rad = registry.any_of<Body>(entity)? registry.get<Body>(entity).radius + weapon.bulletData.rad + 0.001f: 0.0f;
        // Use weapon's bulletData for initialization
        registry.emplace<Position>(bullet, Position{pos.value + dir * rad});
        registry.emplace<Velocity>(bullet, Velocity{dir * weapon.bulletData.speed});
        registry.emplace<HP>(bullet, HP{weapon.bulletData.hp, weapon.bulletData.hp});
        registry.emplace<Damage>(bullet, Damage{weapon.bulletData.dmg});
        registry.emplace<Body>(bullet, Body{weapon.bulletData.rad, weapon.bulletData.color});
        registry.emplace<Lifetime>(bullet, weapon.bulletData.lifetime);

        weapon.timeSinceLastShot = 0.0f;
    }
}
