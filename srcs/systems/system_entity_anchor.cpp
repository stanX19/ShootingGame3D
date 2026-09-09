#include "systems.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/anchor.hpp"

void systems::EntityAnchor::update(GameContext& context, float dt) {
	auto parentView = context.registry.view<const Position, const Rotation>();

	for (auto [entity, anchor, pos] : context.registry.view<const PositionAnchor, Position>().each()) {
		if (parentView.contains(anchor.parent)) {
			const auto& parentPos = parentView.get<const Position>(anchor.parent).value;
			const auto& parentRot = parentView.get<const Rotation>(anchor.parent).value;
			const Vector3 prevPos = pos.value;
			context.registry.emplace_or_replace<PrevPosition>(entity, pos.value);
			pos.value = parentPos + Vector3RotateByQuaternion(anchor.relpos, parentRot);
			context.registry.emplace_or_replace<Velocity>(entity, (pos.value - prevPos) / dt);
		}
	}

	for (auto [entity, anchor, rot] : context.registry.view<const RotationAnchor, Rotation>().each()) {
		if (parentView.contains(anchor.parent)) {
			const auto& parentRot = parentView.get<const Rotation>(anchor.parent).value;
			const Quaternion prevRot = rot.value;
			context.registry.emplace_or_replace<PrevRotation>(entity, rot.value);
			rot.value = QuaternionMultiply(parentRot, anchor.relrot);
			const Quaternion relRot = QuaternionMultiply(QuaternionInvert(prevRot), rot.value);
			context.registry.emplace_or_replace<RotationVelocity>(entity, QuaternionNormalize(QuaternionScale(relRot, 1.0f / dt)));
		}
	}
}

