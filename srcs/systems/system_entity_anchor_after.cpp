#include "systems.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/anchor.hpp"
#include "components/combat.hpp"

void systems::EntityAnchorRelease::update(GameContext& context, float dt) {
	for (auto [entity, anchor, pos, prevPos] : context.registry.view<const PositionAnchor, const Position, const PrevPosition>().each()) {
		if (!context.registry.valid(anchor.parent)) {
			context.registry.emplace_or_replace<Velocity>(entity, Velocity{(pos.value - prevPos.value) / dt});
			context.registry.remove<PositionAnchor>(entity);
		}
	}

	for (auto [entity, anchor] : context.registry.view<const DeathAnchor>().each()) {
		if (!context.registry.valid(anchor.parent)) {
			context.registry.emplace_or_replace<::DelayedDamage>(entity, anchor.delay, 10000000000.0f);
			context.registry.remove<DeathAnchor>(entity);
		}
	}
}

