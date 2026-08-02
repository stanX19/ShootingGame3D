#include "utils.hpp"
#include "includes.hpp"
#include <algorithm>
#include <limits>

/*
D(t) = |(posA + velA * t) - (posB + velB * t)|²
	 = |(relPos + relVel * t)|²
	 = (relPos + relVel * t) • (relPos + relVel * t)
	 = relPos • relPos + 2 * t * (relPos • relVel) + t² * (relVel • relVel)
	 = a*t² + b*t + c

	 		 /‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\
	_______t1____colliding____t2_____> time
		  /					   \

TL;DR: t1 = start of collision; t2 = end of collision
*/
std::optional<CollisionInterval> calculateCollisionInterval(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance)
{
	Vector3 relPos = posA - posB;
	Vector3 relVel = velA - velB;

	float a = Vector3DotProduct(relVel, relVel);
	float b = 2.0f * Vector3DotProduct(relPos, relVel);
	float c = Vector3DotProduct(relPos, relPos) - collisionDistance * collisionDistance;

	float discriminant = b * b - 4.0f * a * c;

	if (discriminant < 0.0f || a == 0.0f) {
		// overlapping or no collision
		if (c > 0.0001f)
			return std::nullopt;
		return CollisionInterval{0.0f, std::numeric_limits<float>::infinity()};
	}

	float sqrtD = sqrtf(discriminant);
	float denominator = 2.0f * a;
	float collisionStartDt = (-b - sqrtD) / denominator;
	float collisionEndDt = (-b + sqrtD) / denominator;

	return CollisionInterval{collisionStartDt, collisionEndDt};
}

float calculateCollisionTime(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance)
{
	std::optional<CollisionInterval> interval = calculateCollisionInterval(
		posA,
		velA,
		posB,
		velB,
		collisionDistance
	);

	if (!interval)
		return -1.0f;

	// return (t1 + t2) / 2.0f; // closest point
	if (interval->collisionEndDt < 0.0f)
		return interval->collisionEndDt; // collision was in the past
	if (interval->collisionStartDt < 0.0f)  // and t2 >= 0
		return 0.0f; // already overlapping
	return interval->collisionStartDt; // collision in future
}

bool willCollide(const std::optional<CollisionInterval> &interval, float maxDt)
{
	if (!interval)
		return false;
	if (interval->collisionEndDt < 0.0f)
		return false;

	float collisionDt = std::max(interval->collisionStartDt, 0.0f);
	return willCollide(collisionDt, maxDt);
}

bool willCollide(const Vector3 &posA, const Vector3 &velA,
				 const Vector3 &posB, const Vector3 &velB,
				 float collisionDistance, float maxDt)
{
	std::optional<CollisionInterval> interval = calculateCollisionInterval(
		posA,
		velA,
		posB,
		velB,
		collisionDistance
	);
	return willCollide(interval, maxDt);
}

bool willCollide(float collisionDt, float maxDt)
{
	return (collisionDt >= 0.0f && collisionDt <= maxDt);
}

// Predictive aiming: calculate direction to lead a moving target
// Returns a normalized direction vector
Vector3 calculateLeadDirection(
	const Vector3 &shooterPos,
	const Vector3 &targetPos,
	const Vector3 &targetVel,
	float projectileSpeed)
{
	Vector3 toTarget = targetPos - shooterPos;
	Vector3 relVel = targetVel;

	float a = Vector3LengthSqr(relVel) - projectileSpeed * projectileSpeed;
	float b = 2.0f * Vector3DotProduct(toTarget, relVel);
	float c = Vector3LengthSqr(toTarget);

	float discriminant = b * b - 4.0f * a * c;

	if (discriminant < 0 || fabsf(a) < 1e-5f)
	{
		return Vector3Normalize(toTarget);
	}

	float sqrtDisc = sqrtf(discriminant);
	float t1 = (-b + sqrtDisc) / (2.0f * a);
	float t2 = (-b - sqrtDisc) / (2.0f * a);
	float interceptTime = fminf(t1, t2);

	if (interceptTime < 0.0f)
		interceptTime = fmaxf(t1, t2); // try the other root

	if (interceptTime < 0.0f) // still invalid
		return Vector3Normalize(toTarget);

	Vector3 aimPos = targetPos + targetVel * interceptTime;
	return Vector3Normalize(aimPos - shooterPos);
}


// Run every frame to get the desired forward direction
// use case: chaserVel = calculateVelocityBiasedDirection(...) * speed;
Vector3 calculateVelocityBiasedDirection(
    const Vector3& chaserPos,
    const Vector3& targetPos,
    const Vector3& targetVel,
    float chaserSpeed
) {
	Vector3 finalDir = targetPos - chaserPos;

	for (int i = 0; i < 5; i++) {
		float dist = Vector3Length(finalDir);
		float chaseTime = dist / chaserSpeed;
		Vector3 futureTargetPos = targetPos + targetVel * chaseTime;
		finalDir = futureTargetPos - chaserPos;
	}
	return Vector3Normalize(finalDir);
}
