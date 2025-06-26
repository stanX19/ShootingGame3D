#include "systems.hpp"

void ecs_systems::WeaponUpdateCooldown(GameContext &context, float dt)
{	
	auto cooldownView = context.registry.view<WeaponCooldown>();
	for (auto entity : cooldownView) {
		auto& cooldown = cooldownView.get<WeaponCooldown>(entity);

		cooldown.timeSinceLastShot += dt;
	}
}