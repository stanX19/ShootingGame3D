#include "utils.hpp"
#include "shoot_3d.hpp"
#include <cmath>
#include <iostream>
// Utility functions for direction vectors

Vector3 GetForwardVector(const Rotation &rotation)
{
	return Vector3Transform({0, 0, 1}, QuaternionToMatrix(rotation.value));
}

Vector3 GetUpVector(const Rotation &rotation)
{
	return Vector3Transform({0, 1, 0}, QuaternionToMatrix(rotation.value));
}

Vector3 GetRightVector(const Rotation &rotation)
{
	return Vector3Transform({1, 0, 0}, QuaternionToMatrix(rotation.value));
}

Quaternion RotateAroundAxis(const Quaternion &current, const Vector3 &axis, float angle)
{
	Quaternion q = QuaternionFromAxisAngle(Vector3Normalize(axis), angle);
	return QuaternionNormalize(QuaternionMultiply(q, current));
}

float WrapAngle(float angle)
{
	while (angle < -PI)
		angle += 2.0f * PI;
	while (angle > PI)
		angle -= 2.0f * PI;
	return angle;
}

Quaternion vector3ToRotation(const Vector3 &vec)
{
	Vector3 dir = Vector3Normalize(vec);
	float yaw = atan2f(dir.x, dir.z);
	float pitch = -asinf(dir.y);
	return QuaternionFromEuler(pitch, yaw, 0);
}

// in degrees [0, 180]
float angleDifference(const Quaternion &a, const Quaternion &b)
{
    float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    dot = Clamp(dot, -1.0f, 1.0f);
    float angleRad = 2.0f * acosf( fabsf(dot) );
    return RAD2DEG * angleRad;
}

float angleDifference(const Rotation &a, const Rotation &b)
{
	return angleDifference(a.value, b.value);
}

float angleDifference(const Quaternion &a, const Rotation &b)
{
	return angleDifference(a, b.value);
}

float angleDifference(const Rotation &a, const Quaternion &b)
{
	return angleDifference(a.value, b);
}

float randomFloat(float min = -1.0f, float max = 1.0f)
{
	return min + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / (max - min));
}

Vector3 randomUnitVector3()
{
	Vector3 v{randomFloat(), randomFloat(), randomFloat()};
	return Vector3Normalize(v);
}



Vector3 randomPosInField()
{
	float x = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	float z = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	float y = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	return Vector3{x, y, z};
}