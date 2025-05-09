#include "utils.hpp"
#include <cmath>
#include <iostream>
// Utility functions for direction vectors

Vector3 GetForwardVector(const Rotation& rotation) {
    return Vector3Transform({0, 0, -1}, QuaternionToMatrix(rotation.value));
}

Vector3 GetUpVector(const Rotation& rotation) {
    return Vector3Transform({0, 1, 0}, QuaternionToMatrix(rotation.value));
}

Vector3 GetRightVector(const Rotation& rotation) {
    return Vector3Transform({1, 0, 0}, QuaternionToMatrix(rotation.value));
}

Quaternion RotateAroundAxis(const Quaternion& current, const Vector3& axis, float angle) {
    Quaternion q = QuaternionFromAxisAngle(Vector3Normalize(axis), angle);
    return QuaternionNormalize(QuaternionMultiply(q, current));
}

float WrapAngle(float angle) {
    while (angle < -PI) angle += 2.0f * PI;
    while (angle >  PI) angle -= 2.0f * PI;
    return angle;
}

Quaternion vector3ToRotation(const Vector3& forward, Vector3 up) {
    Vector3 f = Vector3Normalize(forward);
    Vector3 r = Vector3Normalize(Vector3CrossProduct(up, f));
    Vector3 u = Vector3CrossProduct(f, r);

    Matrix m = {
        r.x, r.y, r.z, 0.0f,
        u.x, u.y, u.z, 0.0f,
        f.x, f.y, f.z, 0.0f,
        0,   0,   0,   1.0f
    };

    return QuaternionNormalize(QuaternionFromMatrix(m));
}

Color colorMix(Color a, Color b, float weightA, float weightB) {
    float totalWeight = weightA + weightB;
    if (totalWeight <= 0.0f) totalWeight = 1.0f; // prevent divide by zero

    float wa = weightA / totalWeight;
    float wb = weightB / totalWeight;

    Color result;
    result.r = (unsigned char)(a.r * wa + b.r * wb);
    result.g = (unsigned char)(a.g * wa + b.g * wb);
    result.b = (unsigned char)(a.b * wa + b.b * wb);
    result.a = (unsigned char)(a.a * wa + b.a * wb);
    return result;
}

Color colorRevert(Color a) {
    Color result;
    result.r = (unsigned char)(255 - a.r);
    result.g = (unsigned char)(255 - a.g);
    result.b = (unsigned char)(255 - a.b);
    result.a = (unsigned char)(a.a);
    return result;
}

float randomFloat(float min = -1.0f, float max = 1.0f) {
    return min + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / (max - min));
}

Vector3 randomUnitVector3() {
    Vector3 v{ randomFloat(), randomFloat(), randomFloat() };
    return Vector3Normalize(v);
}