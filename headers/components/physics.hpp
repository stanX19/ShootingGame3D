#pragma once

#include "includes.hpp"

struct Position
{
	Vector3 value = {0, 0, 0};
};

struct PrevPosition
{
	Vector3 value;
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
