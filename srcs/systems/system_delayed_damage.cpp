#include "systems.hpp"

void ecs_systems::delayedDamage(GameContext &context, float dt) {
	for (const auto &[entity, delayedDamage] : context.registry.view<DelayedDamage>().each()) {
		delayedDamage.timeRemaining -= dt;
	}

	for (const auto &[entity, delayedDamage] : context.registry.view<DelayedDamage>(entt::exclude<HP>).each()) {
		context.registry.remove<DelayedDamage>(entity);
	}

	for (const auto &[entity, delayedDamage, hp] : context.registry.view<DelayedDamage, HP>().each()) {
		if (delayedDamage.timeRemaining <= 0) {
			hp.value -= delayedDamage.damage;
			context.registry.remove<DelayedDamage>(entity);
		}
	}
}