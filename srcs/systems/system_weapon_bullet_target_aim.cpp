#include "systems.hpp"
#include "utils.hpp"

void ecs_systems::bulletTargetAim(GameContext &context)
{
	auto view = context.registry.view<Position, AimDirection, AimTarget, BulletWeapon>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		AimDirection &aimDirection = view.get<AimDirection>(entity);
		AimTarget &aimTarget = view.get<AimTarget>(entity);
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);

		if (!aimTargetExists(context, aimTarget))
		{
			if (context.registry.all_of<Rotation>(entity))
				aimDirection.value = GetForwardVector(context.registry.get<Rotation>(entity));
			continue;
		}
		if (!context.registry.all_of<Position, Velocity>(aimTarget.entity))
			continue;
		Position &targetPosition = context.registry.get<Position>(aimTarget.entity);
		Velocity &targetVelocity = context.registry.get<Velocity>(aimTarget.entity);

		aimDirection.value = calculateLeadDirection(
			position.value,
			targetPosition.value,
			targetVelocity.value,
			bulletWeapon.bulletData.speed
		);
	}
}
