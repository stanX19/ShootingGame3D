#include "shoot_3d.hpp"

void ecs_systems::bulletTargetAim(entt::registry &registry)
{
	auto view = registry.view<Position, AimDirection, AimTarget, BulletWeapon>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		AimDirection &aimDirection = view.get<AimDirection>(entity);
		AimTarget &aimTarget = view.get<AimTarget>(entity);
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);

		if (!aimTargetExists(registry, aimTarget))
		{
			if (registry.all_of<Rotation>(entity))
				aimDirection.value = GetForwardVector(registry.get<Rotation>(entity));
			continue;
		}
		if (!registry.all_of<Position, Velocity>(aimTarget.entity))
			continue;
		Position &targetPosition = registry.get<Position>(aimTarget.entity);
		Velocity &targetVelocity = registry.get<Velocity>(aimTarget.entity);

		aimDirection.value = calculateLeadDirection(
			position.value,
			targetPosition.value,
			targetVelocity.value,
			bulletWeapon.bulletData.speed
		);
	}
}
