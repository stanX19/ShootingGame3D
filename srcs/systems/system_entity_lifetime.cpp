#include "systems.hpp"
#include "game_context.hpp"
#include "components/lifetime.hpp"
#include "components/effect.hpp"
#include <vector>

void systems::EntityLifetime::update(GameContext &context, float dt) {
	auto view = context.registry.view<Lifespan>();
	std::vector<entt::entity> toDestroy;

	for (auto entity : view) {
		Lifespan& lifetime = view.get<Lifespan>(entity);
		lifetime.value -= dt;

		if (lifetime.value <= 0) {
			toDestroy.push_back(entity);
		}
	}

	for (auto entity : toDestroy) {
		if (context.registry.valid(entity)) {
			if (!context.registry.any_of<tag::PostDeath>(entity)) {
				auto simpleTrailPtr = context.registry.try_get<effect::HasSimpleTrail>(entity);
				if (simpleTrailPtr && simpleTrailPtr->count > 0 && simpleTrailPtr->maxNodes > 2) {
					auto dummy = context.registry.create();
					context.registry.emplace<effect::HasSimpleTrail>(dummy, *simpleTrailPtr);
					context.registry.emplace<Lifespan>(dummy, simpleTrailPtr->maxAge);
					context.registry.emplace<tag::PostDeath>(dummy);
				}
				auto multiTrailPtr = context.registry.try_get<effect::HasMultiTrail>(entity);
				if (multiTrailPtr && multiTrailPtr->emitterCount > 0) {
					auto dummy = context.registry.create();
					context.registry.emplace<effect::HasMultiTrail>(dummy, *multiTrailPtr);
					context.registry.emplace<Lifespan>(dummy, multiTrailPtr->maxAge);
					context.registry.emplace<tag::PostDeath>(dummy);
				}
			}
			context.registry.destroy(entity);
		}
	}
}