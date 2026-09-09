#include "systems.hpp"
#include "game_context.hpp"
#include "components/weapon.hpp"

void systems::WeaponUpdateCanFire::update(GameContext &context, [[maybe_unused]] float dt)
{
	// mark all weapons as CanFire
	for (auto entity : context.registry.view<tag::weapon::IsWeapon>()) {
		context.registry.emplace_or_replace<tag::weapon::CanFire>(entity);
	}

	// Remove CanFire if cooldown not ready
	for (auto [entity, cooldown] : context.registry.view<const WeaponCooldown, tag::weapon::IsWeapon>().each()) {
		if (cooldown.timeSinceLastShot < cooldown.shootCooldown) {
			context.registry.remove<tag::weapon::CanFire>(entity);
		}
	}

	// Remove CanFire if out of ammo
	for (auto [entity, ammo] : context.registry.view<const Ammo, tag::weapon::IsWeapon>(entt::exclude<ChargedWeapon>).each()) {
		if (ammo.value < 1.0f) {
			context.registry.remove<tag::weapon::CanFire>(entity);
		}
	}

	for (auto [entity, ammo, charge] : context.registry.view<const Ammo, const ChargedWeapon, tag::weapon::IsWeapon>().each()) {
		const float ammoNeeded = charge.chargeAmmo * (1.0f - charge.currentCharge / charge.totalChargeNeeded);
		if (ammo.value < ammoNeeded - 1e-6f) {
			context.registry.remove<tag::weapon::CanFire>(entity);
		}
	}
}

