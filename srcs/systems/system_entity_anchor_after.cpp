#include "systems.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/anchor.hpp"
#include "components/combat.hpp"

void systems::EntityAnchorRelease::update(GameContext& context, float dt) {
	for (auto [entity, anchor, pos, prevPos] : context.registry.view<PositionAnchor, Position, PrevPosition>().each()) {
		if (!context.registry.valid(anchor.parent)) {
			context.registry.emplace_or_replace<Velocity>(entity, Velocity{(pos.value - prevPos.value) / dt});
			context.registry.remove<PositionAnchor>(entity);
		}
	}

	for (auto [entity, anchor] : context.registry.view<DeathAnchor>().each()) {
		if (!context.registry.valid(anchor.parent)) {
			context.registry.emplace_or_replace<::DelayedDamage>(entity, anchor.delay, 10000000000.0f);
			context.registry.remove<DeathAnchor>(entity);
		}
	}
}

