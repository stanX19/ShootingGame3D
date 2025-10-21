#include "systems.hpp"
#include "entities.hpp"

void ecs_systems::hpCleanup(GameContext &context) {
	auto view = context.registry.view<HP>();
	std::vector<entt::entity> toDestroy;

	for (auto entity : view) {
		HP& hp = view.get<HP>(entity);
		if (hp.value <= 0.001f) {
			toDestroy.push_back(entity);
		}
	}

	for (auto entity : toDestroy) {
		if (!context.registry.valid(entity)) continue;

		auto [posPtr, bodyPtr, velPtr] = context.registry.try_get<Position, RenderBody, Velocity>(entity);
		if (posPtr && bodyPtr) {
			float scale = std::cbrt(bodyPtr->scale.x * bodyPtr->scale.y * bodyPtr->scale.z);
			int count = static_cast<int>(std::sqrt(scale)) * 25;
			if (context.registry.any_of<tag::effect::DropDebris>(entity))
				spawnDebris(context, posPtr->value, scale, bodyPtr->color, count, 5.0, velPtr? velPtr->value: Vector3Zeros);
			entt::entity parent = entt::null;
			if (context.registry.any_of<ScoreParent>(entity))
				parent = context.registry.get<ScoreParent>(entity).parent;
			if (context.registry.any_of<tag::effect::ExplodeOnDeath>(entity))
				spawnExplosion(context, posPtr->value, scale * 10, velPtr? velPtr->value: Vector3Zeros, parent);
		}

		context.registry.destroy(entity);
	}
}