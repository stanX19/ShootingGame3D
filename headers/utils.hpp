#ifndef UTILS_HPP
#define UTILS_HPP
#include "includes.hpp"
#include "components.hpp"

// Utility functions
Vector3 GetForwardVector(const Rotation& rotation);
Vector3 GetForwardVector(const Vector3& rotationValue);
Vector3 GetRightVector(const Rotation& rotation);
Vector3 GetUpVector();
Vector3 GetUpVector(const Rotation& rotation);
float WrapAngle(float angle);
Vector3 vectorToRotation(const Vector3& dir);
Color colorMix(Color a, Color b, float weightA = 1.0f, float weightB = 1.0f);
Color colorRevert(Color a);

#endif