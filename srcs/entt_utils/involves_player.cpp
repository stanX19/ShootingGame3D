#include "entt_utils.hpp"
#include "components/score.hpp"

namespace entt_utils {
	entt::entity getRootScoreParent(const entt::registry &registry, entt::entity entity) {
		while (registry.valid(entity)) {
			const auto *scoreParent = registry.try_get<score::ScoreParent>(entity);
			if (!scoreParent || !registry.valid(scoreParent->parent))
				break;
			entity = scoreParent->parent;
		}
		return entity;
	}
}
