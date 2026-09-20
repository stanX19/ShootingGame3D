#ifndef COMPONENTS_COLLISION_HPP
#define COMPONENTS_COLLISION_HPP

#include "includes.hpp"
#include "collision_body_manager.hpp"

namespace collision {

struct CollisionBody
{
	float radius;
};

struct CollisionBodyModel
{
	t_collision_mesh_id modelID;
};

struct Assembly
{
	entt::entity root = entt::null;
};

} // namespace collision

using ::collision::CollisionBody;
using ::collision::CollisionBodyModel;
using ::collision::Assembly;

#endif // COMPONENTS_COLLISION_HPP

