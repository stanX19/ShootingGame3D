#include "systems.hpp"
#include <iostream>

void ecs_systems::entityAnchor(GameContext& context, float dt) {
	auto parentView = context.registry.view<Position, Rotation>();

	for (auto [entity, anchor, pos] : context.registry.view<PositionAnchor, Position>().each()) {
		if (parentView.contains(anchor.parent)) {
			const auto& parentPos = parentView.get<Position>(anchor.parent).value;
			const auto& parentRot = parentView.get<Rotation>(anchor.parent).value;
			Vector3 prevPos = pos.value;
			context.registry.emplace_or_replace<PrevPosition>(entity, pos.value);
			pos.value = parentPos + Vector3RotateByQuaternion(anchor.relpos, parentRot);
			context.registry.emplace_or_replace<Velocity>(entity, (pos.value - prevPos) / dt);
		}
	}

	for (auto [entity, anchor, rot] : context.registry.view<RotationAnchor, Rotation>().each()) {
		if (parentView.contains(anchor.parent)) {
			const auto& parentRot = parentView.get<Rotation>(anchor.parent).value;
			Quaternion prevRot = rot.value;
			context.registry.emplace_or_replace<PrevRotation>(entity, rot.value);
			rot.value = QuaternionMultiply(parentRot, anchor.relrot);
			Quaternion relRot = QuaternionMultiply(QuaternionInvert(prevRot), rot.value);
			context.registry.emplace_or_replace<RotationVelocity>(entity, QuaternionNormalize(QuaternionScale(relRot, 1.0f / dt)));
		}
	}
}
