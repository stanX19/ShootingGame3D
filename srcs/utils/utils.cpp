#include "utils.hpp"
#include <cmath>
#include <iostream>
// Utility functions for direction vectors
Vector3 GetForwardVector(const Rotation& rotation) {
    // Calculate forward vector using sine and cosine directly for more reliable results
    return GetForwardVector(rotation.value);
}

Vector3 GetForwardVector(const Vector3& rotationValue) {
    // Calculate forward vector using sine and cosine directly for more reliable results
    float sp = sinf(rotationValue.x);
    float cp = cosf(rotationValue.x);
    float sy = sinf(rotationValue.y);
    float cy = cosf(rotationValue.y);

    return Vector3Normalize((Vector3) {
        -sy * cp,   // x component
        -sp,        // y component
        -cy * cp    // z component
    });
}

Vector3 GetRightVector(const Rotation& rotation) {
    // Right vector is perpendicular to forward and up
    return Vector3CrossProduct(GetUpVector(rotation), GetForwardVector(rotation));
}

Vector3 GetUpVector() {
    // Using world up vector
    return (Vector3) { 0, 1, 0 };
}

Vector3 GetUpVector(const Rotation& rotation) {
    float sp = sinf(rotation.value.x);
    float cp = cosf(rotation.value.x);
    float sy = sinf(rotation.value.y);
    float cy = cosf(rotation.value.y);

    return Vector3Normalize((Vector3) {
        -sy * sp,  // x component
         cp,       // y component
        -cy * sp   // z component
    });
}

float WrapAngle(float angle) {
    while (angle < -PI) angle += 2.0f * PI;
    while (angle >  PI) angle -= 2.0f * PI;
    return angle;
}

Vector3 vector3ToRotation(const Vector3& dir) {
    Vector3 rot;
    rot.x = -asinf(dir.y);                    // Inverted pitch
    rot.y = atan2f(-dir.x, -dir.z);           // Inverted yaw to match -Z forward
    rot.z = 0.0f;
    return rot;
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