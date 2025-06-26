#include "systems.hpp"

void ecs_systems::entityAnchor(GameContext& context) {
    std::unordered_map<entt::entity, std::pair<Vector3, Quaternion>> parentTransforms;

    // Cache parents
    for (auto [entity, pos, rot] : context.registry.view<Position, Rotation>().each()) {
		parentTransforms.emplace(entity, std::make_pair(pos.value, rot.value));
	}

    // PositionAnchor
    for (auto [entity, anchor, pos] : context.registry.view<PositionAnchor, Position>().each()) {
        auto it = parentTransforms.find(anchor.parent);
        if (it == parentTransforms.end()) continue;

        const auto& [parentPos, parentRot] = it->second;
        pos.value = parentPos + Vector3RotateByQuaternion(anchor.relpos, parentRot);
    }

    // RotationAnchor
    for (auto [entity, anchor, rot] : context.registry.view<RotationAnchor, Rotation>().each()) {
        auto it = parentTransforms.find(anchor.parent);
        if (it == parentTransforms.end()) continue;

        const auto& parentRot = it->second.second;
        rot.value = QuaternionMultiply(parentRot, anchor.relrot);
    }
}
