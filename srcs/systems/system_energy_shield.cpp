#include "systems.hpp"

namespace {
	void shieldRegen(GameContext &context, float dt) {
		auto view = context.registry.view<EnergyShield, EnergyShieldRegen>();

		for (auto [entity, shield, regen] : view.each()) {
			if (shield.activeTimer > -regen.regenCd)
				continue;
			shield.hp = Clamp(shield.hp + regen.value * dt, 0, shield.maxHp);
		}
	}

	void shieldActiveTimer(GameContext &context, float dt) {
		auto view = context.registry.view<EnergyShield>();

		for (auto [entity, shield] : view.each()) {
			shield.activeTimer -= dt;
			if (shield.activeTimer > 0 && shield.hp <= 0.001f)
				shield.activeTimer = 0.0f;
		}
	}
}

void ecs_systems::energyShield(GameContext &context, float dt) {
	shieldActiveTimer(context, dt);
	shieldRegen(context, dt);
}
