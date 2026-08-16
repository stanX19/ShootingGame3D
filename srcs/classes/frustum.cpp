#include "frustum.hpp"
#include <cmath>
#include <algorithm>

Frustum Frustum::fromViewProjection(const Matrix &m)
{
	Frustum f;
	// Gribb-Hartmann OpenGL plane extraction from column-major View-Projection matrix
	// Left plane: row 3 + row 0
	f.planes[0] = { m.m3 + m.m0, m.m7 + m.m4, m.m11 + m.m8, m.m15 + m.m12 };
	// Right plane: row 3 - row 0
	f.planes[1] = { m.m3 - m.m0, m.m7 - m.m4, m.m11 - m.m8, m.m15 - m.m12 };
	// Bottom plane: row 3 + row 1
	f.planes[2] = { m.m3 + m.m1, m.m7 + m.m5, m.m11 + m.m9, m.m15 + m.m13 };
	// Top plane: row 3 - row 1
	f.planes[3] = { m.m3 - m.m1, m.m7 - m.m5, m.m11 - m.m9, m.m15 - m.m13 };
	// Near plane: row 3 + row 2
	f.planes[4] = { m.m3 + m.m2, m.m7 + m.m6, m.m11 + m.m10, m.m15 + m.m14 };
	// Far plane: row 3 - row 2
	f.planes[5] = { m.m3 - m.m2, m.m7 - m.m6, m.m11 - m.m10, m.m15 - m.m14 };

	// Normalize all plane equations
	for (int i = 0; i < 6; i++) {
		float len = std::sqrt(f.planes[i].a * f.planes[i].a +
		                      f.planes[i].b * f.planes[i].b +
		                      f.planes[i].c * f.planes[i].c);
		if (len > 0.0f) {
			float invLen = 1.0f / len;
			f.planes[i].a *= invLen;
			f.planes[i].b *= invLen;
			f.planes[i].c *= invLen;
			f.planes[i].d *= invLen;
		}
	}
	return f;
}

bool Frustum::isSphereInside(const Vector3 &center, float radius) const
{
	for (int i = 0; i < 6; i++) {
		if (planes[i].a * center.x + planes[i].b * center.y + planes[i].c * center.z + planes[i].d < -radius)
			return false;
	}
	return true;
}

bool Frustum::isPointInside(const Vector3 &point) const
{
	return isSphereInside(point, 0.0f);
}
