#include "shoot_3d.hpp"

bool	aimTargetExists(GameContext &context, AimTarget &target) {
	return context.registry.valid(target.entity);
}