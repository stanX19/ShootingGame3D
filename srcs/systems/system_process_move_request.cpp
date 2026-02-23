#include "systems.hpp"
#include "utils.hpp"
#include <cmath>

void ecs_systems::processMoveRequest(GameContext &context, float dt) {
    for (auto [entity, vel, tVel] : context.registry.view<Velocity, TargetVelocity>().each()) {
        vel.value = Vector3Lerp(vel.value, tVel.value, Clamp(tVel.lerpSpeed * dt, 0.0f, 1.0f));
    }

    for (auto [entity, rot, tRot] : context.registry.view<Rotation, TargetRotation>(entt::exclude<TurnSpeed>).each()) {
        rot.value = QuaternionSlerp(rot.value, tRot.value, Clamp(tRot.slerpSpeed * dt, 0.0f, 1.0f));
    }

	for (auto [entity, rot, tRot, turnSpeed] : context.registry.view<Rotation, TargetRotation, TurnSpeed>().each()) {
		float angleDeg = angleDifference(rot, tRot.value);
		if (angleDeg < 0.001f) {
			rot.value = tRot.value;
			continue;
		}
		float maxAngleTurned = turnSpeed.value * RAD2DEG * dt;
		float slerpAmount = maxAngleTurned / angleDeg;
		rot.value = QuaternionSlerp(rot.value, tRot.value, Clamp(slerpAmount, 0.0f, 1.0f));
    }

	for (auto [entity, rot, tRot, turnSpeed, vel] : context.registry.view<Rotation, TargetRotation, TurnSpeed, Velocity, tag::VelocitySyncRot>().each()) {
		vel.value = getForwardVector(rot.value) * Vector3Length(vel.value);
    }
}
