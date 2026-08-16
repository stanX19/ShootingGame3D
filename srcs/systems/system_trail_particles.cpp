#include "systems.hpp"
#include "entities.hpp"

void ecs_systems::spawnTrailParticles(GameContext &context, [[maybe_unused]] float dt) {
	for (auto [entity, trail, pos, vel] : context.registry.view<SpawnsTrailParticles, Position, Velocity>().each()) {
		if (Vector3Length(vel.value) < 10.0f)
			continue;

		const Rotation* rotation = context.registry.try_get<Rotation>(entity);
		const Quaternion orientation = rotation != nullptr
			? rotation->value
			: QuaternionIdentity();
		for (std::size_t index = 0; index < trail.spawnCount; ++index) {
			entt::entity particle = context.registry.create();
			const Vector3 worldSpawn = pos.value
				+ Vector3RotateByQuaternion(
					trail.spawnLocations[index],
					orientation
				);

			context.registry.emplace<Position>(particle, Position{worldSpawn});
			context.registry.emplace<Velocity>(particle, Velocity{vel.value * 0.5f});
			context.registry.emplace<RenderBody>(
				particle,
				context.modelManager.createSphere(6, 6),
				ColorAlpha(trail.color, 0.5f),
				trail.radius
			);
			context.registry.emplace<RadiusExpand>(
				particle,
				-trail.radius / trail.lifespan * 0.75f
			);
			context.registry.emplace<Lifespan>(particle, trail.lifespan);
		}
	}
}
