#include "systems.hpp"
#include "game_context.hpp"
#include "components/combat.hpp"

void systems::HpRegen::update(GameContext &context, float dt) {
	auto view = context.registry.view<HP, const HPRegen>();

	for (auto entity : view) {
		HP& hp = view.get<HP>(entity);
		const HPRegen& regen = view.get<const HPRegen>(entity);
		
		hp.value = Clamp(hp.value + regen.value * dt, 0, hp.maxValue);
	}
}

