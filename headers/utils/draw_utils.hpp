#ifndef DRAW_UTILS_HPP
#define DRAW_UTILS_HPP

#include "includes.hpp"

namespace draw_utils {
	bool isInFrontOfCamera(const Vector3 &entityPos, const Camera3D &camera);
}

#endif