#include "systems.hpp"

void ecs_systems::entityAnchor(GameContext& context) {
	auto parentView = context.registry.view<Position, Rotation>();

	for (auto [entity, anchor, pos] : context.registry.view<PositionAnchor, Position>().each()) {
		if (parentView.contains(anchor.parent)) {
			const auto& parentPos = parentView.get<Position>(anchor.parent).value;
			const auto& parentRot = parentView.get<Rotation>(anchor.parent).value;
			context.registry.emplace_or_replace<PrevPosition>(entity, pos.value);
			pos.value = parentPos + Vector3RotateByQuaternion(anchor.relpos, parentRot);
		}
	}

	for (auto [entity, anchor, rot] : context.registry.view<RotationAnchor, Rotation>().each()) {
		if (parentView.contains(anchor.parent)) {
			const auto& parentRot = parentView.get<Rotation>(anchor.parent).value;
			rot.value = QuaternionMultiply(parentRot, anchor.relrot);
		}
	}
}
