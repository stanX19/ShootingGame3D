#ifndef UTILS_HPP
#define UTILS_HPP
#include "includes.hpp"
#include "components.hpp"

// Utility functions
Vector3 GetForwardVector(const Rotation& rotation);
Vector3 GetRightVector(const Rotation& rotation);
Vector3 GetUpVector();
Vector3 GetUpVector(const Rotation& rotation);
float WrapAngle(float angle);

#endif