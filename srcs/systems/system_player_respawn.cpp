#include "systems.hpp"
#include "entities.hpp"
#include "components/factions.hpp"
#include "components/unit_camera.hpp"

void ecs_systems::playerRespawn(GameContext &context, [[maybe_unused]] float dt) {
	if (context.registry.valid(context.currentPlayer))
		return;
	entt::entity closest = entt::null;
	float closestDistSq = MAXFLOAT;

	for (auto [entity, pos, faction]: context.registry.view<Position, tag::Spaceship, faction::Faction>().each()) {
		if (faction.value != faction::FAC_BLUE)
			continue ;
		float distSq = Vector3LengthSqr(Vector3Subtract(pos.value, context.mainCamera.target));
		if (distSq < closestDistSq) {
			closestDistSq = distSq;
			closest = entity;
		}
	}
	if (closest != entt::null) {
		context.currentPlayer = closest;
		context.registry.emplace_or_replace<camera::UnitCamera>(closest);
		context.registry.emplace_or_replace<tag::weapon::PlayerControlledFire>(closest);
		context.registry.remove<tag::AIMoveControl>(closest);
		context.registry.remove<tag::weapon::AIControlledFire>(closest);
	}
}