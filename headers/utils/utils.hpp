#ifndef UTILS_HPP
#define UTILS_HPP
#include "includes.hpp"
#include "components.hpp"

// Utility functions
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
Vector3 randomPosInField();
Quaternion randomRotation();
Matrix getTransformMatrix(const Vector3 &scale, const Vector3 &rotation, const Vector3 &displacement);

float angleDifference(const Vector3 &a, const Vector3 &b);
float angleDifference(const Quaternion& a, const Quaternion& b);
float angleDifference(const Rotation& a, const Rotation& b);
float angleDifference(const Quaternion& a, const Rotation& b);
float angleDifference(const Rotation& a, const Quaternion& b);

float wrapAngle(float angle);
float wrapAngleDegree(float angle);
float randomFloat(float min = -1.0f, float max = 1.0f);

// color
Color colorRevert(Color a);

// algorithms
Vector3 calculateLeadDirection(const Vector3 &shooterPos, const Vector3 &targetPos, const Vector3 &targetVel, float projectileSpeed);
bool willCollide(float collisionDt, float maxDt);
bool willCollide(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance, float maxDt);
float calculateCollisionTime(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance);
std::string getParentDir(const std::string &path);
std::string getFileName(const std::string &path);

Vector2 getMouseRatioRelCenter();
Vector2 getMouseDirectionNormalized(float clampRatio = 1.0);
#endif
