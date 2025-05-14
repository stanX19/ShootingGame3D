#include "shoot_3d.hpp"

static Vector3 getEntityAimNormalized(entt::registry &registry, entt::entity e)
{
	if (registry.all_of<AimDirection>(e))
	{
		return registry.get<AimDirection>(e).value;
	}
	if (registry.all_of<Rotation>(e))
	{
		return GetForwardVector(registry.get<Rotation>(e));
	}
	return {0, 0, 1}; // Default forward
}

void ecs_systems::bulletWeaponShoot(entt::registry &registry, float dt)
{
	auto view = registry.view<BulletWeapon, Position>();

	for (auto entity : view)
	{
		auto &weapon = view.get<BulletWeapon>(entity);
		auto &pos = view.get<Position>(entity);

		weapon.timeSinceLastShot += dt;

		if (!weapon.firing || weapon.timeSinceLastShot < weapon.shootCooldown)
			continue;

		if (registry.any_of<Ammo>(entity))
		{
			auto &ammo = registry.get<Ammo>(entity);
			if (ammo.value < 1.0f)
				continue;

			ammo.value = std::max(ammo.value - 1.0f, 0.0f);
		}
		Vector3 dir = getEntityAimNormalized(registry, entity);
		float rad = registry.any_of<CollisionBody>(entity) ? registry.get<CollisionBody>(entity).radius + weapon.bulletData.rad + 0.001f : 0.0f;
		
		spawnBullet(
			registry,
			Position{pos.value + dir * rad},
			Velocity{dir * weapon.bulletData.speed},
			HP{weapon.bulletData.hp},
			Damage{weapon.bulletData.dmg},
			weapon.bulletData.rad,
			weapon.bulletData.color,
			Lifetime{weapon.bulletData.lifetime}
		);

		weapon.timeSinceLastShot = 0.0f;
	}
}
