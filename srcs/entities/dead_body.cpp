#include "entities.hpp"
#include "components/effect.hpp"
#include "components/lifetime.hpp"
#include "components/identity.hpp"

entt::entity spawnDeadBody(GameContext &context, entt::entity dyingEntity) {
	if (!context.registry.valid(dyingEntity)) {
		return entt::null;
	}
	if (context.registry.any_of<tag::PostDeath>(dyingEntity)) {
		return entt::null;
	}

	const auto [simpleTrail, multiTrail] = context.registry.try_get<
		effect::HasSimpleTrail,
		effect::HasMultiTrail
	>(dyingEntity);

	const bool hasSimple = (simpleTrail != nullptr && simpleTrail->count > 0 && simpleTrail->maxNodes > 2);
	const bool hasMulti = (multiTrail != nullptr && multiTrail->emitterCount > 0);

	if (!hasSimple && !hasMulti) {
		return entt::null;
	}

	const entt::entity deadBody = context.registry.create();
	context.registry.emplace_or_replace<tag::PostDeath>(deadBody);

	const float lifespan = (context.config.deathBodyLifespan > 0.0f)
		? context.config.deathBodyLifespan
		: 0.35f;

	if (hasSimple) {
		context.registry.emplace_or_replace<effect::HasSimpleTrail>(deadBody, *simpleTrail);
	}
	if (hasMulti) {
		context.registry.emplace_or_replace<effect::HasMultiTrail>(deadBody, *multiTrail);
	}

	context.registry.emplace_or_replace<Lifespan>(deadBody, lifespan);
	return deadBody;
}
