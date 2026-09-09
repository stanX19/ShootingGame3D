#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include "includes.hpp"

class Frustum {
public:
	struct Plane {
		float a, b, c, d;
	};

	Frustum() = default;

	static Frustum fromViewProjection(const Matrix &viewProj);

	bool isSphereInside(const Vector3 &center, float radius) const;
	bool isPointInside(const Vector3 &point) const;

private:
	Plane m_planes[6]; // Left, Right, Bottom, Top, Near, Far
};

#endif // FRUSTUM_HPP
