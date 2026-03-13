#ifndef DRAW_UTILS_HPP
#define DRAW_UTILS_HPP

#include "includes.hpp"

namespace draw_utils {
	bool isInFrontOfCamera(const Vector3 &entityPos, const Camera3D &camera);
	bool draw_text_button(const char *text, Rectangle bounds, Color color, int fontSize = 20);
}

#endif