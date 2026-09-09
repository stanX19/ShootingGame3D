#pragma once

#include "includes.hpp"
#include "components/physics.hpp"

Quaternion rotateAroundAxis(const Quaternion& current, const Vector3& axis, float angle);
Vector3 getForwardVector(const Rotation& rotation);
Vector3 getForwardVector(const Quaternion &rotation);
Vector3 getRightVector(const Rotation& rotation);
Vector3 getRightVector(const Quaternion &rotation);
Vector3 getUpVector(const Rotation& rotation);
Vector3 getUpVector(const Quaternion &rotation);
Quaternion vector3ToRotation(const Vector3& forward);
Quaternion vector3ToRotation(const Vector3& forward, const Vector3& up);
Quaternion vector3ToRotation(const Vector3& newForward, const Quaternion &baseRotation);
Vector3 randomUnitVector3();
Quaternion randomRotation();
Matrix getTransformMatrix(const Vector3 &scale, const Vector3 &rotation, const Vector3 &displacement);

float angleDifference(const Vector3 &a, const Vector3 &b);
float angleDifference(const Quaternion& a, const Quaternion& b);
float angleDifference(const Rotation& a, const Rotation& b);
float angleDifference(const Quaternion& a, const Rotation& b);
float angleDifference(const Rotation& a, const Quaternion& b);
