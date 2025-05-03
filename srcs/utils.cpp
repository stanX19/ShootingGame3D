#include "shoot_3d.hpp"
#include <cmath>

// Game constants - adjusted for better gameplay (defined here as they are used in utils)


// Utility functions for direction vectors
Vector3 GetForwardVector(const Rotation& rotation) {
    // Calculate forward vector using sine and cosine directly for more reliable results
    float sp = sinf(rotation.value.x);
    float cp = cosf(rotation.value.x);
    float sy = sinf(rotation.value.y);
    float cy = cosf(rotation.value.y);

    return Vector3Normalize((Vector3) {
        -sy * cp,   // x component
        -sp,        // y component
        -cy * cp    // z component
    });
}

Vector3 GetRightVector(const Rotation& rotation) {
    // Right vector is perpendicular to forward and up
    Vector3 forward = GetForwardVector(rotation);
    return Vector3CrossProduct((Vector3) { 0, 1, 0 }, forward);
}

Vector3 GetUpVector() {
    // Using world up vector
    return (Vector3) { 0, 1, 0 };
}