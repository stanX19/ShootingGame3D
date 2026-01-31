#include "systems.hpp"

void ecs_systems::entityMovement(GameContext &context, float dt) {
	for (auto [entity, velocity, scalarAcceleration] : context.registry.view<Velocity, ScalarAcceleration>(entt::exclude<PositionAnchor>).each()) {
		velocity.value += Vector3Normalize(velocity.value) * (scalarAcceleration.value * dt);
	}

	for (auto [entity, position, velocity] : context.registry.view<Position, Velocity>(entt::exclude<PositionAnchor>).each()) {
		context.registry.emplace_or_replace<PrevPosition>(entity, position.value);
		position.value = position.value + velocity.value * dt;
	}

	for (auto [entity, rotation, rotVel] : context.registry.view<Rotation, RotationVelocity>(entt::exclude<RotationAnchor>).each()) {
		Quaternion delta = QuaternionLerp(QuaternionIdentity(), rotVel.value, dt);
		context.registry.emplace_or_replace<PrevRotation>(entity, rotation.value);
        rotation.value = QuaternionMultiply(rotation.value, delta);
	}
}