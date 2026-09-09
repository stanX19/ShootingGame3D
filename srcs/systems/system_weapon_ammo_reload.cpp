#include "systems.hpp"
#include "game_context.hpp"
#include "components/weapon.hpp"
#include "utils/math_utils.hpp"

void systems::AmmoReload::update(GameContext &context, float dt) {
	for (auto [entity, ammo, regen] : context.registry.view<Ammo, const AmmoRegen>().each()) {
		ammo.value = Clamp(ammo.value + regen.value * dt, 0, ammo.maxValue);
	}

	for (auto [entity, ammo, reload] : context.registry.view<Ammo, ::AmmoReload>().each()) {
		if (reload.timer <= 0.0f)
			ammo.value = ammo.maxValue;
			
		if (ammo.value <= 1e-6f)
			reload.timer -= dt;
		else
			reload.timer = reload.cd;
	}
}
