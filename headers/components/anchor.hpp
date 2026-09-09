#pragma once

#include "includes.hpp"

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
}
