#include "systems.hpp"
#include "entities.hpp"
#include "utils.hpp"
#include <random>

void ecs_systems::bulletWeaponShoot(GameContext &context)
{
	static std::mt19937 rng(std::random_device{}());
	
	auto view = context.registry.view<BulletWeapon, Position, AimDirection, tag::weapon::IsFiring, tag::weapon::CanFire>();

	for (auto entity : view)
	{
		auto &weapon = view.get<BulletWeapon>(entity);
		Vector3 pos = view.get<Position>(entity).value;
		Vector3 baseDir = view.get<AimDirection>(entity).value;

		for (int i = 0; i < weapon.bulletData.bulletCount; i++) {
			// shaking, assume dir is normalised
			std::uniform_real_distribution<float> dist(0, weapon.bulletData.spreadSin);
			Vector3 offest = Vector3Normalize(Vector3CrossProduct(randomUnitVector3(), Vector3Normalize(baseDir))) * dist(rng);
			// std::cout << weapon.bulletData.spreadSin << std::endl;
			Vector3 dir = Vector3Normalize(baseDir + offest);
	
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
				Lifespan{weapon.bulletData.lifetime},
				ScoreParent{entity}
			);
		}
		
		context.registry.emplace_or_replace<JustFired>(entity, JustFired{1});
	}
}
