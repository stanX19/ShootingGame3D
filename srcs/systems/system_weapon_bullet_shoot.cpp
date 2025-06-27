#include "systems.hpp"
#include "entities.hpp"

void ecs_systems::bulletWeaponShoot(GameContext &context)
{
	auto view = context.registry.view<BulletWeapon, Position, AimDirection, tag::weapon::IsFiring, tag::weapon::CanFire>();

	for (auto entity : view)
	{
		auto &weapon = view.get<BulletWeapon>(entity);
		Vector3 &pos = view.get<Position>(entity).value;
		Vector3 &dir = view.get<AimDirection>(entity).value;

		// Vector3 dir = getEntityAimNormalized(context, entity);
		float rad = context.registry.any_of<CollisionBody>(entity) ? context.registry.get<CollisionBody>(entity).radius + weapon.bulletData.rad + 1.0f : 0.0f;
		
		spawnBullet(
			context,
			Position{pos + dir * rad},
			Velocity{dir * weapon.bulletData.speed},
			HP{weapon.bulletData.hp},
			Damage{weapon.bulletData.dmg},
			weapon.bulletData.rad,
			weapon.bulletData.color,
			Lifetime{weapon.bulletData.lifetime}
		);
		
		context.registry.emplace_or_replace<JustFired>(entity, JustFired{1});
	}
}
