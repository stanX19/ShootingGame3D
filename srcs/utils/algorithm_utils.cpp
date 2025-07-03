#include "utils.hpp"
#include "includes.hpp"

bool willCollide(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance, float maxDt)
{
	Vector3 relPos = posA - posB;
	Vector3 relVel = velA - velB;

	float a = Vector3DotProduct(relVel, relVel);
	float b = 2.0f * Vector3DotProduct(relPos, relVel);
	float c = Vector3DotProduct(relPos, relPos) - collisionDistance * collisionDistance;

	float discriminant = b * b - 4 * a * c;

	if (discriminant < 0.0f || a == 0.0f) // if no solution or no relative movement
		return (c <= 0.0001);				  // return (is currently overlaping)

	float sqrtD = sqrtf(discriminant);
	float t1 = (-b - sqrtD) / (2.0f * a);
	float t2 = (-b + sqrtD) / (2.0f * a);

	// Collision happens during the frame or is already colliding
	return (t1 <= maxDt && t2 >= 0.0f);
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

	if (interceptTime < 0.0f)  // still invalid
		return Vector3Normalize(toTarget);

	Vector3 aimPos = targetPos + targetVel * interceptTime;
	return Vector3Normalize(aimPos - shooterPos);
}
