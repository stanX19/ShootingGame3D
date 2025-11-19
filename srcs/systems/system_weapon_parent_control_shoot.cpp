#include "systems.hpp"
#include <iostream>

void ecs_systems::weaponParentControlShoot(GameContext& context, [[maybe_unused]] float dt) {
	auto isFiringView = context.registry.view<tag::weapon::FireRequest>();
	auto view = context.registry.view<WeaponParent, tag::weapon::FollowParentAim>();

	for (auto entity : view) {
		auto& wParent = view.get<WeaponParent>(entity);
		if (isFiringView.contains(wParent.parent)) {
			context.registry.emplace_or_replace<tag::weapon::FireRequest>(entity);
			// std::cout << "Updated: firing" << std::endl;
		}
	}
}
