#include "utils.hpp"
#include "constants.hpp"
#include <cmath>
#include <iostream>

Vector3 getForwardVector(const Rotation &rotation)
{
	return Vector3Transform({0, 0, 1}, QuaternionToMatrix(rotation.value));
}

Vector3 getUpVector(const Rotation &rotation)
{
	return Vector3Transform({0, 1, 0}, QuaternionToMatrix(rotation.value));
}

Vector3 getRightVector(const Rotation &rotation)
{
	return Vector3Transform({1, 0, 0}, QuaternionToMatrix(rotation.value));
}

Vector3 getForwardVector(const Quaternion &rotation)
{
	return Vector3Transform({0, 0, 1}, QuaternionToMatrix(rotation));
}

Vector3 getUpVector(const Quaternion &rotation)
{
	return Vector3Transform({0, 1, 0}, QuaternionToMatrix(rotation));
}

Vector3 getRightVector(const Quaternion &rotation)
{
	return Vector3Transform({1, 0, 0}, QuaternionToMatrix(rotation));
}

Quaternion rotateAroundAxis(const Quaternion &current, const Vector3 &axis, float angle)
{
	Quaternion q = QuaternionFromAxisAngle(Vector3Normalize(axis), angle);
	return QuaternionNormalize(QuaternionMultiply(q, current));
}

float WrapAngle(float angle)
{
	while (angle < -PI)
		angle += 2.0f * PI;
	while (angle > PI)
		angle -= 2.0f * PI;
	return angle;
}

Quaternion vector3ToRotation(const Vector3 &vec)
{
	Vector3 dir = Vector3Normalize(vec);
	float yaw = atan2f(dir.x, dir.z);
	float pitch = -asinf(dir.y);
	return QuaternionFromEuler(pitch, yaw, 0);
}

Quaternion vector3ToRotation(const Vector3 &vec, const Vector3 &up)
{
	Vector3 dir = Vector3Normalize(vec);
	Vector3 right = Vector3Normalize(Vector3CrossProduct(up, dir));
	Vector3 correctedUp = Vector3CrossProduct(dir, right);

	Matrix mat = {
		right.x, right.y, right.z, 0.0f,
		correctedUp.x, correctedUp.y, correctedUp.z, 0.0f,
		dir.x, dir.y, dir.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};

	return QuaternionFromMatrix(mat);
}

Quaternion vector3ToRotation(const Vector3 &newForward, const Quaternion &baseRotation)
{
	Vector3 forward = Vector3Normalize(newForward);
	Vector3 oldForward = getForwardVector(baseRotation);

	Quaternion deltaRot = QuaternionFromVector3ToVector3(oldForward, forward);
	return QuaternionMultiply(deltaRot, baseRotation);
}

// in degrees [0, 180]
float angleDifference(const Quaternion &a, const Quaternion &b)
{
	float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	dot = Clamp(dot, -1.0f, 1.0f);
	float angleRad = 2.0f * acosf(fabsf(dot));
	return RAD2DEG * angleRad;
}

float angleDifference(const Rotation &a, const Rotation &b)
{
	return angleDifference(a.value, b.value);
}

float angleDifference(const Quaternion &a, const Rotation &b)
{
	return angleDifference(a, b.value);
}

float angleDifference(const Rotation &a, const Quaternion &b)
{
	return angleDifference(a.value, b);
}

float randomFloat(float min = -1.0f, float max = 1.0f)
{
	return min + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / (max - min));
}

Vector3 randomUnitVector3()
{
	Vector3 v{randomFloat(), randomFloat(), randomFloat()};
	return Vector3Normalize(v);
}

Vector3 randomPosInField()
{
	float x = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	float z = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	float y = GetRandomValue(-ARENA_SIZE / 2 + 5, ARENA_SIZE / 2 - 5);
	return Vector3{x, y, z};
}

// unit quaternion, uniform
Quaternion randomRotation()
{
	float u1 = randomFloat(0.0f, 1.0f);
	float u2 = randomFloat(0.0f, 2.0f * PI);
	float u3 = randomFloat(0.0f, 2.0f * PI);

	float sqrt1 = sqrtf(1.0f - u1);
	float sqrt2 = sqrtf(u1);

	Quaternion q;
	q.x = sqrt1 * sinf(u2);
	q.y = sqrt1 * cosf(u2);
	q.z = sqrt2 * sinf(u3);
	q.w = sqrt2 * cosf(u3);

	return QuaternionNormalize(q); // Just in case
}

Matrix getTransformMatrix(const Vector3 &scale,
						  const Vector3 &rotation,
						  const Vector3 &displacement)
{
	Matrix scaleMatrix = MatrixScale(scale.x, scale.y, scale.z);
	Matrix rotationMatrix = MatrixRotateXYZ(rotation);
	Matrix translationMatrix = MatrixTranslate(displacement.x, displacement.y, displacement.z);

	// Apply transformations in order: Scale -> Rotate -> Translate
	return MatrixMultiply(MatrixMultiply(scaleMatrix, rotationMatrix), translationMatrix);
}