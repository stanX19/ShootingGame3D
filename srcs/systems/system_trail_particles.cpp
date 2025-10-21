#include "systems.hpp"
#include "entities.hpp"

void ecs_systems::spawnTrailParticles(GameContext &context, [[maybe_unused]] float dt) {
	for (auto [entity, trail, pos, vel] : context.registry.view<SpawnsTrailParticle, Position, Velocity>().each()) {
		if (Vector3Length(vel.value) < 10.0f)
			return;
		entt::entity particle = context.registry.create();

		context.registry.emplace<Position>(particle, Position{pos.value});
		context.registry.emplace<Velocity>(particle, Velocity{vel.value * 0.5f});
		context.registry.emplace<RenderBody>(particle, context.modelManager.createSphere(), ColorAlpha(trail.color, 0.5f), trail.radius);
		context.registry.emplace<RadiusExpand>(particle, -trail.radius / trail.lifespan * 0.75f);
		context.registry.emplace<Lifespan>(particle, trail.lifespan);
	}
}

// void ecs_systems::spawnTrailParticles(GameContext &context, [[maybe_unused]] float dt) {
// 	for (auto [entity, trail, pos, vel] : context.registry.view<SpawnsTrailParticle, Position, Velocity>().each()) {
// 		if (Vector3Length(vel.value) < 10.0f)
// 			return;
// 		float radiusExpand = -trail.radius / trail.lifespan;
// 		float dist = Vector3Length(vel.value) * dt;
// 		Vector3 direction = Vector3Normalize(vel.value) * -1.0f;
// 		float curPos = 0;

// 		while(curPos < dist) {
// 			Vector3 particlePos = pos.value + direction * curPos;

// 			entt::entity particle = context.registry.create();
// 			float radius = trail.radius + radiusExpand * dt * curPos / dist;

// 			context.registry.emplace<Position>(particle, Position{particlePos});
// 			context.registry.emplace<Velocity>(particle, Velocity{vel.value * 0.5f});
// 			context.registry.emplace<RenderBody>(particle, context.modelManager.createSphere(), ColorAlpha(SKYBLUE, 0.5f), radius);
// 			context.registry.emplace<RadiusExpand>(particle, radiusExpand);
// 			context.registry.emplace<Lifespan>(particle, trail.lifespan);

// 			curPos += trail.radius;
// 		}
// 	}
// }