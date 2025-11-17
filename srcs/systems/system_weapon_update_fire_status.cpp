#include "systems.hpp"

void ecs_systems::weaponUpdateFireStatus(GameContext &context, [[maybe_unused]] float dt)
{
	// deal with cooldown
	auto cooldownView = context.registry.view<JustFired, WeaponCooldown>();

	for (auto entity : cooldownView) {
		auto& cooldown = cooldownView.get<WeaponCooldown>(entity);

		cooldown.timeSinceLastShot = 0;
	}

	// deal with ammo
	auto ammoView = context.registry.view<JustFired, Ammo>();

	for (auto entity : ammoView) {
		auto& ammo = ammoView.get<Ammo>(entity);
		auto& justFired = ammoView.get<JustFired>(entity);

		ammo.value -= justFired.ammoCount;
	}

	// remove the tag
	auto justFiredView = context.registry.view<JustFired>();

	for (auto entity : justFiredView) {
		context.registry.remove<JustFired>(entity);
	}
}