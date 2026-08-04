#include "systems.hpp"
#include "entities.hpp"

void ecs_systems::hpCleanup(GameContext &context, [[maybe_unused]] float dt) {
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

		auto [posPtr, bodyPtr, velPtr, scoreParentPtr, explodePtr, instantDamagePtr] = context.registry.try_get<
			Position,
			RenderBody,
			Velocity,
			ScoreParent,
			effect::ExplodeOnDeath,
			effect::InstantDamageOnDeath
		>(entity);

		if (!posPtr) {
			context.registry.destroy(entity);
			continue;
		}

		const Vector3 velocity = velPtr ? velPtr->value : Vector3Zeros;
		entt::entity parent = scoreParentPtr ? scoreParentPtr->parent : entt::null;

		if (bodyPtr && context.registry.any_of<tag::effect::DropDebris>(entity)) {
			spawnDebris(context, posPtr->value, bodyPtr, 5.0f, velocity);
		}
		if (explodePtr) {
			spawnExplosion(context, posPtr->value, *explodePtr, velocity, parent);
		}
		if (instantDamagePtr) {
			spawnInstantDamage(context, posPtr->value, *instantDamagePtr, parent);
		}

		context.registry.destroy(entity);
	}
}
