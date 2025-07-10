#include "systems.hpp"

void ecs_systems::entityLifetime(GameContext &context, float dt) {
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
			context.registry.destroy(entity);
		}
	}
}