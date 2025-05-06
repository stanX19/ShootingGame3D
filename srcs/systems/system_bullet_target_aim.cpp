#include "shoot_3d.hpp"

void ecs_systems::bulletTargetAim(entt::registry &registry)
{
	auto view = registry.view<Position, AimRotation, AimTarget, BulletWeapon>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		AimRotation &aimRotation = view.get<AimRotation>(entity);
		AimTarget &aimTarget = view.get<AimTarget>(entity);
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);

		if (!aimTargetExists(registry, aimTarget))
		{
			if (registry.all_of<Rotation>(entity))
				aimRotation.value = registry.get<Rotation>(entity).value;
			continue;
		}
		if (!registry.all_of<Position, Velocity>(aimTarget.entity))
			continue;
		Position &targetPosition = registry.get<Position>(aimTarget.entity);
		Velocity &targetVelocity = registry.get<Velocity>(aimTarget.entity);

		Vector3 leadDirection = calculateLeadDirection(
			position.value,
			targetPosition.value,
			targetVelocity.value,
			bulletWeapon.bulletData.speed
		);
		aimRotation.value = vector3ToRotation(leadDirection);
	}
}
