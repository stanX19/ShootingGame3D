#pragma once

#include "includes.hpp"

struct MaxSpeed
{
	float value;
};

struct TurnSpeed
{
	float value;
};

struct TargetVelocity
{
	Vector3 value = { 0, 0, 0 };
	float lerpSpeed = 30.0f;
};

struct TargetRotation
{
	Quaternion value = QuaternionIdentity();
	float slerpSpeed = 30.0f;
};

struct MoveTarget
{
	entt::entity entity = entt::null;
};
