#include "systems.hpp"

void ecs_systems::ammoReload(GameContext &context, float dt) {
	for (auto [entity, ammo, regen] : context.registry.view<Ammo, AmmoRegen>().each()) {
		ammo.value = Clamp(ammo.value + regen.value * dt, 0, ammo.maxValue);
	}

	for (auto [entity, ammo, reload] : context.registry.view<Ammo, AmmoReload>().each()) {
		if (reload.timer <= 0.0f)
			ammo.value = ammo.maxValue;
			
		if (ammo.value == 0)
			reload.timer -= dt;
		else
			reload.timer = reload.cd;
	}
}
