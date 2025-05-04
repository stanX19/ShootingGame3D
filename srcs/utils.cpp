#include "utils.hpp"
#include <cmath>

// Game constants - adjusted for better gameplay (defined here as they are used in utils)


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

