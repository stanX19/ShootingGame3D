#include "systems.hpp"
#include "game_context.hpp"

bool	aimTargetExists(GameContext &context, AimTarget &target) {
	return context.registry.valid(target.entity);
}