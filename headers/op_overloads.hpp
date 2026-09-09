#ifndef OP_OVERLOADS_HPP
#define OP_OVERLOADS_HPP

#include "includes.hpp"
#include <ostream>

inline std::ostream& operator<<(std::ostream& os, const Vector3& v) {
	return os << v.x << "_" << v.y << "_" << v.z;
}

inline bool operator<(const Matrix& lhs, const Matrix& rhs) {
	const float* a = reinterpret_cast<const float*>(&lhs);
	const float* b = reinterpret_cast<const float*>(&rhs);

	for (int i = 0; i < 16; ++i) {
		if (a[i] != b[i]) return a[i] < b[i];
	}
	return false; // matrices are equal
}

#endif  // OP_OVERLOADS_HPP
