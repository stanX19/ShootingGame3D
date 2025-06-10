#ifndef UTILS_HPP
#define UTILS_HPP
#include "includes.hpp"
#include "components.hpp"

// Utility functions
Quaternion RotateAroundAxis(const Quaternion& current, const Vector3& axis, float angle);
Vector3 GetRightVector(const Rotation& rotation);
Vector3 GetUpVector(const Rotation& rotation);
Vector3 GetForwardVector(const Rotation& rotation);
Quaternion vector3ToRotation(const Vector3& forward);
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
bool aimTargetExists(entt::registry &registry, AimTarget &target);
std::string getParentPath(const std::string &path);
std::string GetFileName(const std::string &path);
#endif
