#include "systems.hpp"

void ecs_systems::weaponUpdateFireStatus(GameContext &context, [[maybe_unused]] float dt)
{
	// deal with cooldown
	for (auto [entity, justFired, cooldown] : context.registry.view<JustFired, WeaponCooldown>().each()) {
		cooldown.timeSinceLastShot = 0;
	}

	// deal with ammo
	for (auto [entity, justFired, ammo] : context.registry.view<JustFired, Ammo>(entt::exclude<ChargedWeapon>).each()) {
		ammo.value -= justFired.ammoCount;
	}

	// remove the tag
	for (auto [entity, justFired] : context.registry.view<JustFired>().each()) {
		context.registry.remove<JustFired>(entity);
	}
}