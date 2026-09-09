#include "math_utils.hpp"
#include <cmath>
#include <iostream>

Vector3 vector3Abs(const Vector3 &vec)
{
	return Vector3{std::abs(vec.x), std::abs(vec.y), std::abs(vec.z)};
}

float wrapAngle(float angle)
{
	while (angle < -PI)
		angle += 2.0f * PI;
	while (angle > PI)
		angle -= 2.0f * PI;
	return angle;
}

float wrapAngleDegree(float angle)
{
	while (angle < -180.0f)
		angle += 2.0f * 180.0f;
	while (angle > 180.0f)
		angle -= 2.0f * 180.0f;
	return angle;
}

float randomFloat(float min, float max)
{
	return min + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / (max - min));
}
