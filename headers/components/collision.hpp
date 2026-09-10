#ifndef COMPONENTS_COLLISION_HPP
#define COMPONENTS_COLLISION_HPP

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

} // namespace collision

using ::collision::CollisionBody;
using ::collision::CollisionBodyModel;

#endif // COMPONENTS_COLLISION_HPP
