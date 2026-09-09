#pragma once

#include "includes.hpp"
#include <optional>
#include <string>

struct CollisionInterval
{
	float collisionStartDt;
	float collisionEndDt;
};

Vector3 calculateLeadDirection(const Vector3 &shooterPos, const Vector3 &targetPos, const Vector3 &targetVel, float projectileSpeed);
Vector3 calculateVelocityBiasedDirection(const Vector3& chaserPos, const Vector3& targetPos, const Vector3& targetVel, float chaserSpeed);
std::optional<CollisionInterval> calculateCollisionInterval(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance);
bool willCollide(const std::optional<CollisionInterval> &interval, float maxDt);
bool willCollide(float collisionDt, float maxDt);
bool willCollide(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance, float maxDt);
float calculateCollisionTime(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance);
