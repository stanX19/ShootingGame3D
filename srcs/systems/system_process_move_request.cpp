#include "systems.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/spaceship.hpp"
#include "components/render.hpp"
#include "utils/vector_rotation_utils.hpp"
#include <cmath>

void systems::ProcessMoveRequest::update(GameContext &context, float dt) {
    for (auto [entity, vel, tVel] : context.registry.view<Velocity, const TargetVelocity>().each()) {
        vel.value = Vector3Lerp(vel.value, tVel.value, Clamp(tVel.lerpSpeed * dt, 0.0f, 1.0f));
    }

    for (auto [entity, rot, tRot] : context.registry.view<Rotation, const TargetRotation>(entt::exclude<TurnSpeed>).each()) {
        rot.value = QuaternionSlerp(rot.value, tRot.value, Clamp(tRot.slerpSpeed * dt, 0.0f, 1.0f));
    }

	for (auto [entity, rot, tRot, turnSpeed] : context.registry.view<Rotation, const TargetRotation, const TurnSpeed>().each()) {
		const float angleDeg = angleDifference(rot, tRot.value);
		if (angleDeg < 0.001f) {
			rot.value = tRot.value;
			continue;
		}
		const float maxAngleTurned = turnSpeed.value * RAD2DEG * dt;
		const float slerpAmount = maxAngleTurned / angleDeg;
		rot.value = QuaternionSlerp(rot.value, tRot.value, Clamp(slerpAmount, 0.0f, 1.0f));
    }

	for (auto [entity, rot, tRot, turnSpeed, vel] : context.registry.view<const Rotation, const TargetRotation, const TurnSpeed, Velocity, tag::VelocitySyncRot>().each()) {
		vel.value = getForwardVector(rot.value) * Vector3Length(vel.value);
    }
}

