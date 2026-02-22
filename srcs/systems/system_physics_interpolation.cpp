#include "systems.hpp"
#include "utils.hpp"
#include <cmath>

void ecs_systems::physicsInterpolation(GameContext &context, float dt) {
    auto view = context.registry.view<Velocity, TargetVelocity>();
    for (auto entity : view) {
        auto &vel = view.get<Velocity>(entity);
        auto &tVel = view.get<TargetVelocity>(entity);
        vel.value = Vector3Lerp(vel.value, tVel.value, Clamp(tVel.lerpSpeed * dt, 0.0f, 1.0f));
    }

    auto rotView = context.registry.view<Rotation, TargetRotation>();
    for (auto entity : rotView) {
        auto &rot = rotView.get<Rotation>(entity);
        auto &tRot = rotView.get<TargetRotation>(entity);
        rot.value = QuaternionSlerp(rot.value, tRot.value, Clamp(tRot.slerpSpeed * dt, 0.0f, 1.0f));
    }
}
