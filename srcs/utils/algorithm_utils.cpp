#include "utils.hpp"
#include "includes.hpp"

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
float calculateCollisionTime(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance)
{
	Vector3 relPos = posA - posB;
	Vector3 relVel = velA - velB;

	float a = Vector3DotProduct(relVel, relVel);
	float b = 2.0f * Vector3DotProduct(relPos, relVel);
	float c = Vector3DotProduct(relPos, relPos) - collisionDistance * collisionDistance;

	float discriminant = b * b - 4.0f * a * c;

	if (discriminant < 0.0f || a == 0.0f)
		return (c <= 0.0001f) ? 0.0f : -1.0f; // overlapping or no collision

	float sqrtD = sqrtf(discriminant);
	float t1 = (-b - sqrtD) / (2.0f * a);
	float t2 = (-b + sqrtD) / (2.0f * a);

	// return (t1 + t2) / 2.0f; // closest point
	if (t2 < 0.0f)
		return t2; // collision was in the past
	if (t1 < 0.0f)
		return 0.0f; // already overlapping
	return t1; // collision in future
}

bool willCollide(const Vector3 &posA, const Vector3 &velA,
				 const Vector3 &posB, const Vector3 &velB,
				 float collisionDistance, float maxDt)
{
	float t = calculateCollisionTime(posA, velA, posB, velB, collisionDistance);
	return willCollide(t, maxDt);
}

bool willCollide(float collisionDt, float maxDt)
{
	return (collisionDt >= 0.0f && collisionDt <= maxDt);
}

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
