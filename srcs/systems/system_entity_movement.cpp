#include "systems.hpp"

void ecs_systems::entityMovement(GameContext &context, float dt) {
	for (auto [entity, position, velocity] : context.registry.view<Position, Velocity>().each()) {
		context.registry.emplace_or_replace<PrevPosition>(entity, position.value);
		position.value = position.value + velocity.value * dt;
	}

	for (auto [entity, rotation, velocity] : context.registry.view<Rotation, RotationVelocity>().each()) {
		Quaternion delta = QuaternionLerp(QuaternionIdentity(), velocity.value, dt);
        rotation.value = QuaternionMultiply(rotation.value, delta);
	}
}