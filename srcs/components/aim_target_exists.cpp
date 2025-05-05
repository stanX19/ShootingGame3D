#include "shoot_3d.hpp"

bool	aimTargetExists(entt::registry &registry, AimTarget &target) {
	return registry.valid(target.entity);
}