#include "systems.hpp"
#include "game_context.hpp"
#include "entities.hpp"
#include "components/combat.hpp"
#include "components/physics.hpp"
#include "components/render.hpp"
#include "components/identity.hpp"
#include "components/score.hpp"
#include "components/effect.hpp"
#include "components/lifetime.hpp"
#include <vector>

void systems::HpCleanup::update(GameContext &context, [[maybe_unused]] float dt) {
	auto view = context.registry.view<const HP>();
	std::vector<entt::entity> toDestroy;

	for (auto entity : view) {
		const HP& hp = view.get<const HP>(entity);
		if (hp.value <= 0.001f) {
			toDestroy.push_back(entity);
		}
	}

	for (auto entity : toDestroy) {
		if (!context.registry.valid(entity)) continue;

		auto [posPtr, bodyPtr, velPtr, ownerPtr, explodePtr, instantDamagePtr] = context.registry.try_get<
			Position,
			RenderBody,
			Velocity,
			identity::Owner,
			effect::ExplodeOnDeath,
			effect::InstantDamageOnDeath
		>(entity);

		if (!posPtr) {
			context.registry.destroy(entity);
			continue;
		}

		const Vector3 velocity = velPtr ? velPtr->value : Vector3Zeros;
		entt::entity parent = ownerPtr ? ownerPtr->root : entt::null;

		if (bodyPtr && context.registry.any_of<tag::effect::DropDebris>(entity)) {
			spawnDebris(context, posPtr->value, bodyPtr, 5.0f, velocity);
		}
		if (explodePtr) {
			spawnExplosion(context, posPtr->value, *explodePtr, velocity, parent);
		}
		if (instantDamagePtr) {
			spawnInstantDamage(context, posPtr->value, *instantDamagePtr, parent);
		}

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

		context.registry.destroy(entity);
	}
}

