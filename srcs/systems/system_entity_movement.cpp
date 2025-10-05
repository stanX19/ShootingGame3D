#include "systems.hpp"

void ecs_systems::entityMovement(GameContext &context, float dt) {
	for (auto [entity, position, velocity] : context.registry.view<Position, Velocity>(entt::exclude<PositionAnchor>).each()) {
		context.registry.emplace_or_replace<PrevPosition>(entity, position.value);
		position.value = position.value + velocity.value * dt;
	}

	for (auto [entity, rotation, velocity] : context.registry.view<Rotation, RotationVelocity>(entt::exclude<RotationAnchor>).each()) {
		Quaternion delta = QuaternionLerp(QuaternionIdentity(), velocity.value, dt);
		context.registry.emplace_or_replace<PrevRotation>(entity, rotation.value);
        rotation.value = QuaternionMultiply(rotation.value, delta);
	}
}