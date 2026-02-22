#include "systems.hpp"
#include "utils.hpp"
#include <cmath>

void ecs_systems::physicsInterpolation(GameContext &context, float dt) {
    for (auto [entity, vel, tVel] : context.registry.view<Velocity, TargetVelocity>().each()) {
        vel.value = Vector3Lerp(vel.value, tVel.value, Clamp(tVel.lerpSpeed * dt, 0.0f, 1.0f));
    }

    for (auto [entity, rot, tRot] : context.registry.view<Rotation, TargetRotation>(entt::exclude<TurnSpeed>).each()) {
        rot.value = QuaternionSlerp(rot.value, tRot.value, Clamp(tRot.slerpSpeed * dt, 0.0f, 1.0f));
    }

	for (auto [entity, rot, tRot, turnSpeed] : context.registry.view<Rotation, TargetRotation, TurnSpeed>().each()) {
		float angleDeg = angleDifference(rot, tRot.value);
		float maxAngleTurned = turnSpeed.value * RAD2DEG * dt;
		float maxSlerp = maxAngleTurned / angleDeg;
		rot.value = QuaternionSlerp(rot.value, tRot.value, Clamp(tRot.slerpSpeed * dt, 0.0f, maxSlerp));
    }
}
