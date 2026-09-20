#ifndef COMPONENTS_PHYSICS_HPP
#define COMPONENTS_PHYSICS_HPP

#include "includes.hpp"

namespace physics {

struct Position
{
	Vector3 value = {0, 0, 0};
	Vector3 prevValue = {0, 0, 0};

	Position() = default;
	Position(Vector3 val) : value(val), prevValue(val) {}
	Position(Vector3 val, Vector3 prev) : value(val), prevValue(prev) {}
};

struct Velocity
{
	Vector3 value = { 0, 0, 0 };
};

struct ScalarAcceleration
{
	float value = 0.0f;
};

struct Rotation
{
	Quaternion value = QuaternionUnitX;
};

struct PrevRotation
{
	Quaternion value = QuaternionUnitX;
};

struct RotationVelocity
{
	Quaternion value = QuaternionIdentity();
};

struct Mass
{
	float value = 1.0f;
};

struct ImpulseRequest
{
	Vector3 value = {0, 0, 0};
};

namespace tag {

struct VelocitySyncRot {};

} // namespace tag

} // namespace physics

namespace tag {
using ::physics::tag::VelocitySyncRot;
} // namespace tag

using ::physics::Position;
using ::physics::Velocity;
using ::physics::ScalarAcceleration;
using ::physics::Rotation;
using ::physics::PrevRotation;
using ::physics::RotationVelocity;
using ::physics::Mass;
using ::physics::ImpulseRequest;

#endif // COMPONENTS_PHYSICS_HPP
