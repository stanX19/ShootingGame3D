#include "systems.hpp"


void ecs_systems::weaponUpdateCanFire(GameContext &context)
{
	// mark all weapons as CanFire
	for (auto entity : context.registry.view<tag::weapon::IsWeapon>()) {
		context.registry.emplace_or_replace<tag::weapon::CanFire>(entity);
	}

	// Remove CanFire if cooldown not ready
	for (auto [entity, cooldown] : context.registry.view<WeaponCooldown, tag::weapon::IsWeapon>().each()) {
		if (cooldown.timeSinceLastShot < cooldown.shootCooldown) {
			context.registry.remove<tag::weapon::CanFire>(entity);
		}
	}

	// Remove CanFire if out of ammo
	for (auto [entity, ammo] : context.registry.view<Ammo, tag::weapon::IsWeapon>().each()) {
		if (ammo.value < 1.0f) {
			context.registry.remove<tag::weapon::CanFire>(entity);
		}
	}
}

