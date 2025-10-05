#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "components/factions.hpp"

void ecs_systems::aiShootControl(GameContext &context) {
	auto view = context.registry.view<Position, AimDirection, AimTarget,
										tag::weapon::IsWeapon, tag::weapon::AIControlledFire>();
	for (auto [entity, position, aimDirection, aimTarget] : view.each())
	{
		if (!aimTargetExists(context, aimTarget) || !context.registry.all_of<Position>(aimTarget.entity)) {
			context.registry.remove<tag::weapon::IsFiring>(entity);
			continue;
		}
		
		Vector3 targetPos = context.registry.get<Position>(aimTarget.entity).value;
		Vector3 toTarget = targetPos - position.value;
		float dist = Vector3Length(toTarget);

		if ((dist < COMBAT_DIST) && Vector3DotProduct(aimDirection.value, Vector3Normalize(toTarget)) > cosf(DEG2RAD * 35.0f)) {
			context.registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
		} else {
			context.registry.remove<tag::weapon::IsFiring>(entity);
		}
	}
}

