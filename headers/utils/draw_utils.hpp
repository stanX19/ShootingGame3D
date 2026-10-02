#ifndef DRAW_UTILS_HPP
#define DRAW_UTILS_HPP

#include "includes.hpp"
#include "components/effect.hpp"

namespace draw_utils {
	bool isInFrontOfCamera(const Vector3 &entityPos, const Camera3D &camera);
	void drawSimpleTrail(const effect::HasSimpleTrail &trail, const Camera3D &camera);
	void drawMultiTrailEmitter(const effect::HasMultiTrail::Emitter &emitter, const effect::HasMultiTrail &trail, const Camera3D &camera);
}

#endif