#include "gen_types.hpp"

#include <cmath>

gen_model::gen_types::Point3 gen_model::gen_types::operator+(
	gen_model::gen_types::Point3 left,
	gen_model::gen_types::Point3 right
) {
	return {left.x + right.x, left.y + right.y, left.z + right.z};
}

gen_model::gen_types::Point3 gen_model::gen_types::operator-(
	gen_model::gen_types::Point3 left,
	gen_model::gen_types::Point3 right
) {
	return {left.x - right.x, left.y - right.y, left.z - right.z};
}

gen_model::gen_types::Point3 gen_model::gen_types::operator*(
	gen_model::gen_types::Point3 point,
	float scalar
) {
	return {point.x * scalar, point.y * scalar, point.z * scalar};
}

float gen_model::gen_types::dot(
	gen_model::gen_types::Point3 left,
	gen_model::gen_types::Point3 right
) {
	return left.x * right.x + left.y * right.y + left.z * right.z;
}

gen_model::gen_types::Point3 gen_model::gen_types::cross(
	gen_model::gen_types::Point3 left,
	gen_model::gen_types::Point3 right
) {
	return {
		left.y * right.z - left.z * right.y,
		left.z * right.x - left.x * right.z,
		left.x * right.y - left.y * right.x
	};
}

gen_model::gen_types::Point3 gen_model::gen_types::normalize(
	gen_model::gen_types::Point3 point
) {
	const float length = std::sqrt(gen_model::gen_types::dot(point, point));
	if (length <= 0.000001f)
		return {0.0f, 1.0f, 0.0f};
	return point * (1.0f / length);
}
