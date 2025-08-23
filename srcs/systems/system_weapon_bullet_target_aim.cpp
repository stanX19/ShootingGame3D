#include "systems.hpp"
#include "utils.hpp"
#include <iostream>

void ecs_systems::bulletTargetAim(GameContext &context)
{
	auto view = context.registry.view<Position, AimDirection, AimTarget, Weapon>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		AimDirection &aimDirection = view.get<AimDirection>(entity);
		AimTarget &aimTarget = view.get<AimTarget>(entity);
		Weapon &bulletWeapon = view.get<Weapon>(entity);
		
		Vector3 targetAimDir = aimDirection.value;
		if (!aimTargetExists(context, aimTarget))
		{
			if (context.registry.all_of<Rotation>(entity))
				targetAimDir = getForwardVector(context.registry.get<Rotation>(entity));
		}
		else if (context.registry.all_of<Position, Velocity>(aimTarget.entity))
		{
			auto [targetPosition, targetVelocity] = context.registry.get<Position, Velocity>(aimTarget.entity);

			targetAimDir = calculateLeadDirection(
				position.value,
				targetPosition.value,
				targetVelocity.value,
				bulletWeapon.bulletData.speed
			);
		}
		
		auto [prevRotPtr, currRotPtr] = context.registry.try_get<PrevRotation, Rotation>(entity);
		if (!prevRotPtr || !currRotPtr) {
			aimDirection.value = targetAimDir;
			continue;
		}
		Vector3 prevRotVec = Vector3RotateByQuaternion(Vector3UnitZ, prevRotPtr->value);
		Vector3 currRotVec = Vector3RotateByQuaternion(Vector3UnitZ, currRotPtr->value);
		Quaternion originalRelRot = QuaternionFromVector3ToVector3(prevRotVec, aimDirection.value);
		Quaternion targetRelRot = QuaternionFromVector3ToVector3(currRotVec, targetAimDir);
		Quaternion newRelRot = QuaternionLerp(originalRelRot, targetRelRot, 0.25);
		aimDirection.value = Vector3RotateByQuaternion(currRotVec, newRelRot);
	}
}
