#ifndef COMPONENTS_ANCHOR_HPP
#define COMPONENTS_ANCHOR_HPP

#include "includes.hpp"

namespace anchor {

struct PositionAnchor
{
	entt::entity parent;
	Vector3 relpos;
};

struct RotationAnchor
{
	entt::entity parent;
	Quaternion relrot = QuaternionUnitX;
};

struct DeathAnchor
{
	entt::entity parent;
	float delay;
};

namespace tag {

struct GetVelOnAnchorDeath {};

} // namespace tag

} // namespace anchor

namespace tag {
using ::anchor::tag::GetVelOnAnchorDeath;
} // namespace tag

using ::anchor::PositionAnchor;
using ::anchor::RotationAnchor;
using ::anchor::DeathAnchor;

#endif // COMPONENTS_ANCHOR_HPP
