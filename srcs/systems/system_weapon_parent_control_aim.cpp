#include "systems.hpp"
#include <iostream>

void ecs_systems::weaponParentControlAim(GameContext& context) {
    auto aimTargetView = context.registry.view<AimTarget>();
	auto view = context.registry.view<WeaponParent, AimTarget, tag::weapon::ParentControlledAim>();

	for (auto entity : view) {
		auto& wParent = view.get<WeaponParent>(entity);
		if (aimTargetView.contains(wParent.parent)) {
			view.get<AimTarget>(entity).entity = aimTargetView.get<AimTarget>(wParent.parent).entity;
		}
	}
}
