#include "frustum.hpp"
#include <cmath>
#include <algorithm>

Frustum Frustum::fromViewProjection(const Matrix &m)
{
	Frustum f;
	// Gribb-Hartmann OpenGL plane extraction from column-major View-Projection matrix
	// Left plane: row 3 + row 0
	f.m_planes[0] = { m.m3 + m.m0, m.m7 + m.m4, m.m11 + m.m8, m.m15 + m.m12 };
	// Right plane: row 3 - row 0
	f.m_planes[1] = { m.m3 - m.m0, m.m7 - m.m4, m.m11 - m.m8, m.m15 - m.m12 };
	// Bottom plane: row 3 + row 1
	f.m_planes[2] = { m.m3 + m.m1, m.m7 + m.m5, m.m11 + m.m9, m.m15 + m.m13 };
	// Top plane: row 3 - row 1
	f.m_planes[3] = { m.m3 - m.m1, m.m7 - m.m5, m.m11 - m.m9, m.m15 - m.m13 };
	// Near plane: row 3 + row 2
	f.m_planes[4] = { m.m3 + m.m2, m.m7 + m.m6, m.m11 + m.m10, m.m15 + m.m14 };
	// Far plane: row 3 - row 2
	f.m_planes[5] = { m.m3 - m.m2, m.m7 - m.m6, m.m11 - m.m10, m.m15 - m.m14 };

	// Normalize all plane equations
	for (int i = 0; i < 6; i++) {
		float len = std::sqrt(f.m_planes[i].a * f.m_planes[i].a +
		                      f.m_planes[i].b * f.m_planes[i].b +
		                      f.m_planes[i].c * f.m_planes[i].c);
		if (len > 0.0f) {
			float invLen = 1.0f / len;
			f.m_planes[i].a *= invLen;
			f.m_planes[i].b *= invLen;
			f.m_planes[i].c *= invLen;
			f.m_planes[i].d *= invLen;
		}
	}
	return f;
}

bool Frustum::isSphereInside(const Vector3 &center, float radius) const
{
	for (int i = 0; i < 6; i++) {
		if (m_planes[i].a * center.x + m_planes[i].b * center.y + m_planes[i].c * center.z + m_planes[i].d < -radius)
			return false;
	}
	return true;
}

bool Frustum::isPointInside(const Vector3 &point) const
{
	return isSphereInside(point, 0.0f);
}
