#include "systems.hpp"
#include <iostream>

void ecs_systems::weaponParentControlAim(GameContext& context) {
    auto aimTargetView = context.registry.view<AimTarget>();
    auto isFiringView = context.registry.view<tag::weapon::IsFiring>();
	auto view = context.registry.view<WeaponParent, AimTarget, tag::weapon::ParentControlledAim>();

	for (auto entity : view) {
		auto& wParent = view.get<WeaponParent>(entity);
		if (aimTargetView.contains(wParent.parent)) {
			view.get<AimTarget>(entity).entity = aimTargetView.get<AimTarget>(wParent.parent).entity;
		}
		if (isFiringView.contains(wParent.parent)) {
			context.registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
			// std::cout << "Updated: firing" << std::endl;
		} else {
			context.registry.remove<tag::weapon::IsFiring>(entity);
			// std::cout << "Updated: not firing" << std::endl;
		}
	}
}
