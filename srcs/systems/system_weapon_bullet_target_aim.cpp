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
		
		Vector3 targetedAimDir = aimDirection.value;
		Velocity *velocityPtr = context.registry.try_get<Velocity>(entity);
		Vector3 shooterVel = velocityPtr ? velocityPtr->value : Vector3Zeros;

		if (aimTargetExists(context, aimTarget) && context.registry.all_of<Position>(aimTarget.entity))
		{
			Vector3 relVel = shooterVel * -1;

			if (context.registry.all_of<Velocity>(aimTarget.entity)) {
				relVel += context.registry.get<Velocity>(aimTarget.entity).value;
			}

			targetedAimDir = calculateLeadDirection(
				position.value,
				context.registry.get<Position>(aimTarget.entity).value,
				relVel,
				bulletWeapon.bulletData.speed
			);
		} else if (context.registry.all_of<Rotation>(entity)) {
			targetedAimDir = getForwardVector(context.registry.get<Rotation>(entity));
		}

		auto [prevRotPtr, currRotPtr] = context.registry.try_get<PrevRotation, Rotation>(entity);
		if (!prevRotPtr || !currRotPtr) {
			aimDirection.value = targetedAimDir;
			continue;
		}
		Vector3 prevRotVec = Vector3RotateByQuaternion(Vector3UnitZ, prevRotPtr->value);
		Vector3 currRotVec = Vector3RotateByQuaternion(Vector3UnitZ, currRotPtr->value);
		Quaternion originalRelRot = QuaternionFromVector3ToVector3(prevRotVec, aimDirection.value);
		Quaternion targetRelRot = QuaternionFromVector3ToVector3(currRotVec, targetedAimDir);
		Quaternion newRelRot = QuaternionLerp(originalRelRot, targetRelRot, 0.25);
		aimDirection.value = Vector3RotateByQuaternion(currRotVec, newRelRot);
	}
}
