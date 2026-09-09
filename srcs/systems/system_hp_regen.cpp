#include "systems.hpp"
#include "game_context.hpp"
#include "components/combat.hpp"

void systems::HpRegen::update(GameContext &context, float dt) {
	auto view = context.registry.view<HP, HPRegen>();

	for (auto entity : view) {
		HP& hp = view.get<HP>(entity);
		HPRegen& regen = view.get<HPRegen>(entity);
		
		hp.value = Clamp(hp.value + regen.value * dt, 0, hp.maxValue);
	}
}

