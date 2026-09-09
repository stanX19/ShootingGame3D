#include "systems.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/anchor.hpp"

void systems::EntityMovement::update(GameContext &context, float dt) {
	for (auto [entity, velocity, scalarAcceleration] : context.registry.view<Velocity, const ScalarAcceleration>(entt::exclude<PositionAnchor>).each()) {
		velocity.value += Vector3Normalize(velocity.value) * (scalarAcceleration.value * dt);
	}

	for (auto [entity, position, velocity] : context.registry.view<Position, const Velocity>(entt::exclude<PositionAnchor>).each()) {
		context.registry.emplace_or_replace<PrevPosition>(entity, position.value);
		position.value = position.value + velocity.value * dt;
	}

	for (auto [entity, rotation, rotVel] : context.registry.view<Rotation, const RotationVelocity>(entt::exclude<RotationAnchor>).each()) {
		const Quaternion delta = QuaternionLerp(QuaternionIdentity(), rotVel.value, dt);
		context.registry.emplace_or_replace<PrevRotation>(entity, rotation.value);
		rotation.value = QuaternionMultiply(rotation.value, delta);
	}
}