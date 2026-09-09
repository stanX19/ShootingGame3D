#include "systems.hpp"
#include "game_context.hpp"
#include "components/weapon.hpp"

void systems::WeaponUpdateCooldown::update(GameContext &context, float dt)
{	
	auto cooldownView = context.registry.view<WeaponCooldown>();
	for (auto entity : cooldownView) {
		auto& cooldown = cooldownView.get<WeaponCooldown>(entity);

		cooldown.timeSinceLastShot += dt;
	}
}