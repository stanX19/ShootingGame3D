#ifndef UTILS_HPP
#define UTILS_HPP
#include "includes.hpp"
#include "components.hpp"

// Utility functions
Quaternion RotateAroundAxis(const Quaternion& current, const Vector3& axis, float angle);
Vector3 GetForwardVector(const Rotation& rotation);
Vector3 GetForwardVector(const Quaternion &rotation);
Vector3 GetRightVector(const Rotation& rotation);
Vector3 GetRightVector(const Quaternion &rotation);
Vector3 GetUpVector(const Rotation& rotation);
Vector3 GetUpVector(const Quaternion &rotation);
Quaternion vector3ToRotation(const Vector3& forward);
Quaternion vector3ToRotation(const Vector3& forward, const Vector3& up);
Quaternion vector3ToRotation(const Vector3& newForward, const Quaternion &baseRotation);
float angleDifference(const Quaternion& a, const Quaternion& b);
float angleDifference(const Rotation& a, const Rotation& b);
float angleDifference(const Quaternion& a, const Rotation& b);
float angleDifference(const Rotation& a, const Quaternion& b);
float WrapAngle(float angle);
Color colorMix(Color a, Color b, float weightA = 1.0f, float weightB = 1.0f);
Color colorRevert(Color a);
Vector3 calculateLeadDirection(const Vector3 &shooterPos, const Vector3 &targetPos, const Vector3 &targetVel, float projectileSpeed);
Vector3 randomUnitVector3();
Vector3 randomPosInField();
Quaternion randomRotation();
std::string getParentPath(const std::string &path);
std::string GetFileName(const std::string &path);

Vector2 getMouseRatioRelCenter();
Vector2 getMouseDirectionNormalized(float clampRatio = 1.0);
#endif
