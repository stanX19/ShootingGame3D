#ifndef GEN_MODEL_TYPES_HPP
#define GEN_MODEL_TYPES_HPP

#include <array>
#include <cstdint>
#include <vector>

namespace gen_model::gen_types {
	struct Point2 {
		float x;
		float y;
		bool operator==(const Point2&) const = default;
	};

	struct Point3 {
		float x;
		float y;
		float z;
		bool operator==(const Point3&) const = default;
	};

	Point3 operator+(Point3 left, Point3 right);
	Point3 operator-(Point3 left, Point3 right);
	Point3 operator*(Point3 point, float scalar);
	float dot(Point3 left, Point3 right);
	Point3 cross(Point3 left, Point3 right);
	Point3 normalize(Point3 point);

	struct Triangle {
		std::array<int, 3> positionIndices;
		std::array<int, 3> texcoordIndices;
		std::array<int, 3> normalIndices;
		bool operator==(const Triangle&) const = default;
	};

	struct MeshData {
		std::vector<Point3> positions;
		std::vector<Point2> texcoords;
		std::vector<Point3> normals;
		std::vector<Triangle> triangles;
	};

	struct TextureData {
		int width = 0;
		int height = 0;
		std::vector<std::uint8_t> rgba;
	};

	struct AssetData {
		MeshData mesh;
		TextureData texture;
		TextureData normalMap;
	};
} // namespace gen_model::gen_types

#endif // GEN_MODEL_TYPES_HPP
