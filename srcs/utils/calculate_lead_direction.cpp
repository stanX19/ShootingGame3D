#include "utils.hpp"

#include "raymath.h"
#include <cmath>
#include <optional>

Vector3 calculateLeadDirection(
	const Vector3 &shooterPos,
	const Vector3 &targetPos,
	const Vector3 &targetVel,
	float projectileSpeed)
{
	Vector3 toTarget = Vector3Subtract(targetPos, shooterPos);
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

	if (interceptTime < 0.0f)
		return Vector3Normalize(toTarget); // still invalid

	Vector3 aimPos = Vector3Add(targetPos, Vector3Scale(targetVel, interceptTime));
	return Vector3Normalize(Vector3Subtract(aimPos, shooterPos));
}
