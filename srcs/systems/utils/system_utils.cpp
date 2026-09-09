#include "systems.hpp"
#include "game_context.hpp"

bool	aimTargetExists(const GameContext &context, const AimTarget &target) {
	return context.registry.valid(target.entity);
}