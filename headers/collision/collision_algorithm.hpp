#ifndef COLLISION_ALGORITHM_HPP
#define COLLISION_ALGORITHM_HPP

#include "collision_body_manager.hpp"
#include "utils.hpp"

#include <optional>

struct CollisionMeshInstance
{
	Vector3 previousPosition;
	Vector3 currentPosition;
	Vector3 translation;
	Vector3 scale;
	Quaternion rotation;
};

struct CollisionHit
{
	float collisionDt;
	Vector3 contactPoint;
	Vector3 contactNormal;
};

std::optional<CollisionHit> sweepSphereAgainstMesh(
	const CollisionModel &mesh,
	const CollisionMeshInstance &meshInstance,
	const Vector3 &spherePreviousPosition,
	const Vector3 &sphereDisplacement,
	float sphereRadius,
	const CollisionInterval &broadPhaseInterval
);

#endif
