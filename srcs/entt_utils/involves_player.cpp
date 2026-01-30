#include "entt_utils.hpp"
#include "components.hpp"

bool entt_utils::involvesPlayer(GameContext &context, entt::entity entity) {
	entt::registry &registry = context.registry;
	while (registry.valid(entity)) {
		if (entity == context.currentPlayer)
			return true;
		auto anchorPtr = registry.try_get<PositionAnchor>(entity);
		if (!anchorPtr)
			return false;
		entity = anchorPtr->parent;
	}
	return false;
}
