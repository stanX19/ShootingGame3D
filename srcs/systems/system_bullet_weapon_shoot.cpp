#include "shoot_3d.hpp"

static Vector3 getEntityAimNormalized(GameContext &context, entt::entity e)
{
	if (context.registry.all_of<AimDirection>(e))
	{
		return context.registry.get<AimDirection>(e).value;
	}
	if (context.registry.all_of<Rotation>(e))
	{
		return GetForwardVector(context.registry.get<Rotation>(e));
	}
	return {0, 0, 1}; // Default forward
}

void ecs_systems::bulletWeaponShoot(GameContext &context, float dt)
{
	auto view = context.registry.view<BulletWeapon, Position>();

	for (auto entity : view)
	{
		auto &weapon = view.get<BulletWeapon>(entity);
		auto &pos = view.get<Position>(entity);

		weapon.timeSinceLastShot += dt;

		if (!weapon.firing || weapon.timeSinceLastShot < weapon.shootCooldown)
			continue;

		if (context.registry.any_of<Ammo>(entity))
		{
			auto &ammo = context.registry.get<Ammo>(entity);
			if (ammo.value < 1.0f)
				continue;

			ammo.value = std::max(ammo.value - 1.0f, 0.0f);
		}
		Vector3 dir = getEntityAimNormalized(context, entity);
		float rad = context.registry.any_of<CollisionBody>(entity) ? context.registry.get<CollisionBody>(entity).radius + weapon.bulletData.rad + 0.001f : 0.0f;
		
		spawnBullet(
			context,
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
