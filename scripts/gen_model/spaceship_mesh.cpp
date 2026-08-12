#include "spaceship_mesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {
	constexpr float PI = 3.14159265358979323846f;

	struct UvRect {
		float minU;
		float minV;
		float maxU;
		float maxV;
	};

	gen_model::gen_types::Point3 divide(gen_model::gen_types::Point3 value, float divisor) {
		return value * (1.0f / divisor);
	}

	gen_model::gen_types::Point3 midpoint(
		gen_model::gen_types::Point3 left,
		gen_model::gen_types::Point3 right
	) {
		return (left + right) * 0.5f;
	}

	UvRect surfaceRect(gen_model::spaceship::detail::Surface surface) {
		constexpr float inset = 0.006f;
		switch (surface) {
			case gen_model::spaceship::detail::Surface::Armor:
				return {inset, inset, 0.5f - inset, 0.5f - inset};
			case gen_model::spaceship::detail::Surface::Structure:
			case gen_model::spaceship::detail::Surface::Socket:
				return {0.5f + inset, inset, 1.0f - inset, 0.5f - inset};
			case gen_model::spaceship::detail::Surface::Canopy:
				return {inset, 0.5f + inset, 0.5f - inset, 1.0f - inset};
			case gen_model::spaceship::detail::Surface::Engine:
				return {0.5f + inset, 0.5f + inset, 1.0f - inset, 1.0f - inset};
		}
		throw std::logic_error("Unknown spaceship surface");
	}

	std::array<gen_model::gen_types::Point2, 4> quadUvs(
		gen_model::spaceship::detail::Surface surface
	) {
		const UvRect rect = surfaceRect(surface);
		return {
			gen_model::gen_types::Point2{rect.minU, rect.minV},
			gen_model::gen_types::Point2{rect.maxU, rect.minV},
			gen_model::gen_types::Point2{rect.maxU, rect.maxV},
			gen_model::gen_types::Point2{rect.minU, rect.maxV}
		};
	}

	std::array<gen_model::gen_types::Point2, 3> triangleUvs(
		gen_model::spaceship::detail::Surface surface
	) {
		const UvRect rect = surfaceRect(surface);
		return {
			gen_model::gen_types::Point2{rect.minU, rect.minV},
			gen_model::gen_types::Point2{rect.maxU, rect.minV},
			gen_model::gen_types::Point2{rect.maxU, rect.maxV}
		};
	}

	gen_model::gen_types::Point2 continuousArmorUv(
		gen_model::gen_types::Point3 point
	) {
		const UvRect rect = surfaceRect(gen_model::spaceship::detail::Surface::Armor);
		// Armor faces used to map the entire atlas quadrant independently.  Every
		// quad therefore repeated the same broad noise and its seams lit as a square
		// grid on the player.  Project the armor skin in ship space instead: adjacent
		// faces now sample one continuous coating while the other material islands
		// retain their deliberately local UV layout.
		constexpr float ARMOR_UV_SPAN = 12.0f;
		const float u = std::clamp(0.5f + point.x / ARMOR_UV_SPAN, 0.0f, 1.0f);
		const float v = std::clamp(0.5f + point.z / ARMOR_UV_SPAN, 0.0f, 1.0f);
		return {
			rect.minU + (rect.maxU - rect.minU) * u,
			rect.minV + (rect.maxV - rect.minV) * v
		};
	}

	void validateSegments(int segments) {
		if (segments < 6 || segments > 64)
			throw std::invalid_argument("Cylinder segments must be between 6 and 64");
	}

	bool isSmoothSurface(gen_model::spaceship::detail::Surface surface) {
		// Armor and canopy are continuous pressure skins.  Their vertices are emitted
		// per face so each material island can retain its own UVs, but their lighting
		// should still flow across shared positions.  Structure, sockets, and engine
		// hardware deliberately keep hard normals for readable mechanical breaks.
		return surface == gen_model::spaceship::detail::Surface::Armor
			|| surface == gen_model::spaceship::detail::Surface::Canopy;
	}

	struct SmoothNormalKey {
		std::int64_t x;
		std::int64_t y;
		std::int64_t z;
		int surface;

		bool operator==(const SmoothNormalKey&) const = default;
	};

	struct SmoothNormalKeyHash {
		std::size_t operator()(const SmoothNormalKey& key) const noexcept {
			std::size_t hash = std::hash<std::int64_t>{}(key.x);
			hash ^= std::hash<std::int64_t>{}(key.y) + 0x9E3779B9u + (hash << 6u) + (hash >> 2u);
			hash ^= std::hash<std::int64_t>{}(key.z) + 0x9E3779B9u + (hash << 6u) + (hash >> 2u);
			hash ^= std::hash<int>{}(key.surface) + 0x9E3779B9u + (hash << 6u) + (hash >> 2u);
			return hash;
		}
	};

	struct WeldPositionKey {
		std::int64_t x;
		std::int64_t y;
		std::int64_t z;

		bool operator==(const WeldPositionKey&) const = default;
	};

	struct WeldPositionKeyHash {
		std::size_t operator()(const WeldPositionKey& key) const noexcept {
			std::size_t hash = std::hash<std::int64_t>{}(key.x);
			hash ^= std::hash<std::int64_t>{}(key.y) + 0x9E3779B9u + (hash << 6u) + (hash >> 2u);
			hash ^= std::hash<std::int64_t>{}(key.z) + 0x9E3779B9u + (hash << 6u) + (hash >> 2u);
			return hash;
		}
	};

	struct FaceKey {
		std::array<int, 3> indices{};

		bool operator==(const FaceKey&) const = default;
	};

	struct FaceKeyHash {
		std::size_t operator()(const FaceKey& key) const noexcept {
			std::size_t hash = std::hash<int>{}(key.indices[0]);
			hash ^= std::hash<int>{}(key.indices[1]) + 0x9E3779B9u + (hash << 6u) + (hash >> 2u);
			hash ^= std::hash<int>{}(key.indices[2]) + 0x9E3779B9u + (hash << 6u) + (hash >> 2u);
			return hash;
		}
	};

	struct PositionEdgeKey {
		int start;
		int end;

		bool operator==(const PositionEdgeKey&) const = default;
	};

	struct PositionEdgeKeyHash {
		std::size_t operator()(const PositionEdgeKey& key) const noexcept {
			std::size_t hash = std::hash<int>{}(key.start);
			hash ^= std::hash<int>{}(key.end) + 0x9E3779B9u + (hash << 6u) + (hash >> 2u);
			return hash;
		}
	};

	struct Bounds {
		gen_model::gen_types::Point3 minimum{
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max()
		};
		gen_model::gen_types::Point3 maximum{
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest()
		};

		void include(gen_model::gen_types::Point3 point) {
			minimum.x = std::min(minimum.x, point.x);
			minimum.y = std::min(minimum.y, point.y);
			minimum.z = std::min(minimum.z, point.z);
			maximum.x = std::max(maximum.x, point.x);
			maximum.y = std::max(maximum.y, point.y);
			maximum.z = std::max(maximum.z, point.z);
			}

		bool contains(gen_model::gen_types::Point3 point, float margin) const {
			return point.x >= minimum.x - margin && point.x <= maximum.x + margin
				&& point.y >= minimum.y - margin && point.y <= maximum.y + margin
				&& point.z >= minimum.z - margin && point.z <= maximum.z + margin;
		}

		bool contains(const Bounds& other, float margin) const {
			return contains(other.minimum, margin) && contains(other.maximum, margin);
		}

		float volume() const {
			const gen_model::gen_types::Point3 extent = maximum - minimum;
			return std::max(0.0f, extent.x)
				* std::max(0.0f, extent.y)
				* std::max(0.0f, extent.z);
		}
	};

	struct TriangleComponent {
		std::vector<std::size_t> triangles;
		Bounds bounds;
		bool closed = true;
	};

	struct PlaneKey {
		std::int64_t normalX;
		std::int64_t normalY;
		std::int64_t normalZ;
		std::int64_t offset;

		bool operator==(const PlaneKey&) const = default;
	};

	struct PlaneKeyHash {
		std::size_t operator()(const PlaneKey& key) const noexcept {
			std::size_t result = 0;
			const auto combine = [&result](std::int64_t value) {
				const std::size_t hashed = std::hash<std::int64_t>{}(value);
				result ^= hashed + 0x9E3779B9u + (result << 6u) + (result >> 2u);
			};
			combine(key.normalX);
			combine(key.normalY);
			combine(key.normalZ);
			combine(key.offset);
			return result;
		}
	};

	struct ProjectedTriangle {
		std::size_t triangleIndex = 0u;
		std::size_t componentIndex = 0u;
		gen_model::gen_types::Point3 normal{};
		float planeOffset = 0.0f;
		std::array<float, 3> minimum{};
		std::array<float, 3> maximum{};
		std::array<std::array<float, 2>, 3> points{};
		float minimumU = 0.0f;
		float maximumU = 0.0f;
		float minimumV = 0.0f;
		float maximumV = 0.0f;
	};

	WeldPositionKey weldPositionKey(gen_model::gen_types::Point3 point) {
		constexpr double SCALE = 100000.0;
		return {
			static_cast<std::int64_t>(std::llround(static_cast<double>(point.x) * SCALE)),
			static_cast<std::int64_t>(std::llround(static_cast<double>(point.y) * SCALE)),
			static_cast<std::int64_t>(std::llround(static_cast<double>(point.z) * SCALE))
		};
	}

	gen_model::gen_types::Point3 triangleNormal(
		const gen_model::spaceship::detail::TaggedMesh& taggedMesh,
		std::size_t triangleIndex
	) {
		const auto& triangle = taggedMesh.mesh.triangles[triangleIndex];
		const auto& a = taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])];
		const auto& b = taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])];
		const auto& c = taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])];
		return gen_model::gen_types::normalize(
			gen_model::gen_types::cross(b - a, c - a)
		);
	}

	PlaneKey planeKey(
		gen_model::gen_types::Point3 normal,
		float offset
	) {
		if (std::abs(normal.x) > 0.000001f
			? normal.x < 0.0f
			: (std::abs(normal.y) > 0.000001f
				? normal.y < 0.0f
				: normal.z < 0.0f)) {
			normal = normal * -1.0f;
			offset = -offset;
		}
		constexpr double QUANTIZATION = 10000.0;
		return {
			static_cast<std::int64_t>(std::llround(normal.x * QUANTIZATION)),
			static_cast<std::int64_t>(std::llround(normal.y * QUANTIZATION)),
			static_cast<std::int64_t>(std::llround(normal.z * QUANTIZATION)),
			static_cast<std::int64_t>(std::llround(offset * QUANTIZATION))
		};
	}

	int dominantNormalAxis(gen_model::gen_types::Point3 normal) {
		const std::array<float, 3> absolute{
			std::abs(normal.x), std::abs(normal.y), std::abs(normal.z)
		};
		return static_cast<int>(std::max_element(absolute.begin(), absolute.end()) - absolute.begin());
	}

	float coordinate(gen_model::gen_types::Point3 point, int axis) {
		switch (axis) {
			case 0: return point.x;
			case 1: return point.y;
			case 2: return point.z;
		}
		throw std::logic_error("Invalid projected spaceship mesh axis");
	}

	ProjectedTriangle projectTriangle(
		const gen_model::spaceship::detail::TaggedMesh& taggedMesh,
		std::size_t triangleIndex,
		std::size_t componentIndex,
		int normalAxis,
		gen_model::gen_types::Point3 normal
	) {
		ProjectedTriangle result;
		result.triangleIndex = triangleIndex;
		result.componentIndex = componentIndex;
		result.normal = normal;
		const auto& triangle = taggedMesh.mesh.triangles[triangleIndex];
		const std::array<int, 2> projectedAxes = normalAxis == 0
			? std::array<int, 2>{1, 2}
			: normalAxis == 1
				? std::array<int, 2>{0, 2}
				: std::array<int, 2>{0, 1};
		result.minimumU = result.minimumV = std::numeric_limits<float>::max();
		result.maximumU = result.maximumV = std::numeric_limits<float>::lowest();
		result.minimum = {
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max()
		};
		result.maximum = {
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest()
		};
		for (std::size_t corner = 0; corner < 3u; ++corner) {
			const auto& point = taggedMesh.mesh.positions[
				static_cast<std::size_t>(triangle.positionIndices[corner])
			];
			result.points[corner] = {
				coordinate(point, projectedAxes[0]),
				coordinate(point, projectedAxes[1])
			};
			result.minimumU = std::min(result.minimumU, result.points[corner][0]);
			result.maximumU = std::max(result.maximumU, result.points[corner][0]);
			result.minimumV = std::min(result.minimumV, result.points[corner][1]);
			result.maximumV = std::max(result.maximumV, result.points[corner][1]);
			for (int axis = 0; axis < 3; ++axis) {
				result.minimum[static_cast<std::size_t>(axis)] = std::min(
					result.minimum[static_cast<std::size_t>(axis)], coordinate(point, axis)
				);
				result.maximum[static_cast<std::size_t>(axis)] = std::max(
					result.maximum[static_cast<std::size_t>(axis)], coordinate(point, axis)
				);
			}
		}
		result.planeOffset = gen_model::gen_types::dot(normal,
			taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])] );
		return result;
	}

	float cross2(
		std::array<float, 2> left,
		std::array<float, 2> right
	) {
		return left[0] * right[1] - left[1] * right[0];
	}

	std::array<float, 2> subtract2(
		std::array<float, 2> left,
		std::array<float, 2> right
	) {
		return {left[0] - right[0], left[1] - right[1]};
	}

	float signedArea2(const std::vector<std::array<float, 2>>& polygon) {
		float result = 0.0f;
		for (std::size_t index = 0; index < polygon.size(); ++index)
			result += cross2(
				polygon[index],
				polygon[(index + 1u) % polygon.size()]
			);
		return result;
	}

	std::array<float, 2> lineIntersection(
		std::array<float, 2> start,
		std::array<float, 2> end,
		std::array<float, 2> edgeStart,
		std::array<float, 2> edgeEnd
	) {
		const auto direction = subtract2(end, start);
		const auto edge = subtract2(edgeEnd, edgeStart);
		const float denominator = cross2(direction, edge);
		if (std::abs(denominator) <= 0.00000001f)
			return end;
		const float amount = cross2(subtract2(edgeStart, start), edge) / denominator;
		return {
			start[0] + direction[0] * amount,
			start[1] + direction[1] * amount
		};
	}

	std::vector<std::array<float, 2>> clipPolygon(
		const std::vector<std::array<float, 2>>& subject,
		std::array<float, 2> edgeStart,
		std::array<float, 2> edgeEnd
	) {
		std::vector<std::array<float, 2>> result;
		if (subject.empty())
			return result;
		const auto edge = subtract2(edgeEnd, edgeStart);
		const auto inside = [&](std::array<float, 2> point) {
			return cross2(edge, subtract2(point, edgeStart)) >= -0.000001f;
		};
		std::array<float, 2> previous = subject.back();
		bool previousInside = inside(previous);
		for (const auto current : subject) {
			const bool currentInside = inside(current);
			if (currentInside != previousInside)
				result.push_back(lineIntersection(previous, current, edgeStart, edgeEnd));
			if (currentInside)
				result.push_back(current);
			previous = current;
			previousInside = currentInside;
		}
		return result;
	}

	bool projectedTrianglesOverlap(const ProjectedTriangle& left, const ProjectedTriangle& right) {
		constexpr float EPSILON = 0.000001f;
		if (left.maximumU <= right.minimumU + EPSILON
			|| right.maximumU <= left.minimumU + EPSILON
			|| left.maximumV <= right.minimumV + EPSILON
			|| right.maximumV <= left.minimumV + EPSILON)
			return false;
		std::vector<std::array<float, 2>> subject(left.points.begin(), left.points.end());
		std::vector<std::array<float, 2>> clipper(right.points.begin(), right.points.end());
		if (signedArea2(subject) < 0.0f)
			std::reverse(subject.begin(), subject.end());
		if (signedArea2(clipper) < 0.0f)
			std::reverse(clipper.begin(), clipper.end());
		for (std::size_t edge = 0; edge < clipper.size(); ++edge)
			subject = clipPolygon(
				subject,
				clipper[edge],
				clipper[(edge + 1u) % clipper.size()]
			);
		// Shared-edge triangles can leave a tiny sliver from float clipping even
		// when their actual intersection has zero area.  Keep the gate above that
		// numerical noise while still rejecting real render-surface overlap.
		return std::abs(signedArea2(subject)) * 0.5f > 0.00001f;
	}

	std::vector<TriangleComponent> buildTriangleComponents(
		const gen_model::spaceship::detail::TaggedMesh& taggedMesh
	);

	void requireNoCoplanarRenderOverlaps(
		const gen_model::spaceship::detail::TaggedMesh& taggedMesh
	) {
		const auto components = buildTriangleComponents(taggedMesh);
		std::vector<std::size_t> triangleComponents(taggedMesh.mesh.triangles.size(), 0u);
		for (std::size_t componentIndex = 0; componentIndex < components.size(); ++componentIndex)
			for (const std::size_t triangleIndex : components[componentIndex].triangles)
				triangleComponents[triangleIndex] = componentIndex;

		std::unordered_map<PlaneKey, std::vector<ProjectedTriangle>, PlaneKeyHash> planes;
		planes.reserve(taggedMesh.mesh.triangles.size());
		for (std::size_t triangleIndex = 0; triangleIndex < taggedMesh.mesh.triangles.size(); ++triangleIndex) {
			const auto normal = triangleNormal(taggedMesh, triangleIndex);
			if (gen_model::gen_types::dot(normal, normal) <= 0.00000001f)
				continue;
			const auto& triangle = taggedMesh.mesh.triangles[triangleIndex];
			const auto& point = taggedMesh.mesh.positions[
				static_cast<std::size_t>(triangle.positionIndices[0])
			];
			const PlaneKey key = planeKey(normal, gen_model::gen_types::dot(normal, point));
			planes[key].push_back(projectTriangle(
				taggedMesh,
				triangleIndex,
				triangleComponents[triangleIndex],
				dominantNormalAxis(normal),
				normal
			));
		}

		for (auto& [key, triangles] : planes) {
			(void)key;
			if (triangles.size() < 2u)
				continue;
			std::sort(triangles.begin(), triangles.end(), [](
				const ProjectedTriangle& left,
				const ProjectedTriangle& right
			) {
				if (left.minimumU != right.minimumU)
					return left.minimumU < right.minimumU;
				return left.triangleIndex < right.triangleIndex;
			});
			for (std::size_t leftIndex = 0; leftIndex < triangles.size(); ++leftIndex) {
				const auto& left = triangles[leftIndex];
				for (std::size_t rightIndex = leftIndex + 1u; rightIndex < triangles.size(); ++rightIndex) {
					const auto& right = triangles[rightIndex];
					if (right.minimumU > left.maximumU + 0.0001f)
						break;
					// Coplanar projection alone is not enough to indicate a render
					// conflict.  Dorsal and ventral structures can legitimately share
					// the same X/Z plane while being separated in Y; depth buffering
					// cannot make those surfaces fight because they are distinct 3D
					// layers.  Only reject overlaps whose 3D bounds actually touch.
					bool separatedIn3D = false;
					for (std::size_t axis = 0; axis < 3u; ++axis)
						separatedIn3D = separatedIn3D
							|| left.maximum[axis] < right.minimum[axis] - 0.0005f
							|| right.maximum[axis] < left.minimum[axis] - 0.0005f;
					if (separatedIn3D)
						continue;
					if (!projectedTrianglesOverlap(left, right))
						continue;
					const auto& leftTag = taggedMesh.tags[left.triangleIndex];
					const auto& rightTag = taggedMesh.tags[right.triangleIndex];
					std::ostringstream message;
					message << "Spaceship render mesh has coplanar overlapping faces: left="
						<< left.triangleIndex << " surface=" << static_cast<int>(leftTag.surface)
						<< " owner=" << leftTag.mountOwner << " right=" << right.triangleIndex
						<< " surface=" << static_cast<int>(rightTag.surface)
						<< " owner=" << rightTag.mountOwner
						<< " normal=(" << left.normal.x << ',' << left.normal.y << ',' << left.normal.z << ')'
						<< " leftPoints=[(" << left.points[0][0] << ',' << left.points[0][1] << ")"
						<< ",(" << left.points[1][0] << ',' << left.points[1][1] << ")"
						<< ",(" << left.points[2][0] << ',' << left.points[2][1] << ")]"
						<< " rightPoints=[(" << right.points[0][0] << ',' << right.points[0][1] << ")"
						<< ",(" << right.points[1][0] << ',' << right.points[1][1] << ")"
						<< ",(" << right.points[2][0] << ',' << right.points[2][1] << ")]";
					throw std::invalid_argument(message.str());
				}
			}
		}
	}

	void requireNoNearCoplanarRenderOverlaps(
		const gen_model::spaceship::detail::TaggedMesh& taggedMesh
	) {
		// Depth buffers cannot reliably distinguish two independent skins that are
		// only a few thousandths of a model unit apart.  Exact-plane checks catch
		// duplicate caps, but small normal-map/detail offsets can still flicker.  Use
		// the same projected positive-area test for nearly parallel surfaces emitted
		// by different components; shared-edge triangles in one component remain
		// legitimate and are excluded.
		const auto components = buildTriangleComponents(taggedMesh);
		std::vector<std::size_t> triangleComponents(taggedMesh.mesh.triangles.size(), 0u);
		for (std::size_t componentIndex = 0; componentIndex < components.size(); ++componentIndex)
			for (const std::size_t triangleIndex : components[componentIndex].triangles)
				triangleComponents[triangleIndex] = componentIndex;

		std::unordered_map<PlaneKey, std::vector<ProjectedTriangle>, PlaneKeyHash> normalBuckets;
		normalBuckets.reserve(taggedMesh.mesh.triangles.size());
		for (std::size_t triangleIndex = 0; triangleIndex < taggedMesh.mesh.triangles.size(); ++triangleIndex) {
			const auto normal = triangleNormal(taggedMesh, triangleIndex);
			if (gen_model::gen_types::dot(normal, normal) <= 0.00000001f)
				continue;
			PlaneKey key = planeKey(normal, 0.0f);
			key.offset = 0;
			normalBuckets[key].push_back(projectTriangle(
				taggedMesh,
				triangleIndex,
				triangleComponents[triangleIndex],
				dominantNormalAxis(normal),
				normal
			));
		}

		constexpr float NORMAL_DOT_MINIMUM = 0.9995f;
		constexpr float PLANE_DISTANCE = 0.0035f;
		constexpr float AABB_MARGIN = 0.0005f;
		for (auto& [key, triangles] : normalBuckets) {
			(void)key;
			std::sort(triangles.begin(), triangles.end(), [](
				const ProjectedTriangle& left,
				const ProjectedTriangle& right
			) {
				if (left.planeOffset != right.planeOffset)
					return left.planeOffset < right.planeOffset;
				return left.triangleIndex < right.triangleIndex;
			});
			for (std::size_t leftIndex = 0; leftIndex < triangles.size(); ++leftIndex) {
				const auto& left = triangles[leftIndex];
				for (std::size_t rightIndex = leftIndex + 1u; rightIndex < triangles.size(); ++rightIndex) {
					const auto& right = triangles[rightIndex];
					if (right.planeOffset - left.planeOffset > PLANE_DISTANCE)
						break;
					if (left.componentIndex == right.componentIndex
						|| std::abs(gen_model::gen_types::dot(left.normal, right.normal)) < NORMAL_DOT_MINIMUM)
						continue;
					bool separatedIn3D = false;
					for (std::size_t axis = 0; axis < 3u; ++axis)
						separatedIn3D = separatedIn3D
							|| left.maximum[axis] < right.minimum[axis] - AABB_MARGIN
							|| right.maximum[axis] < left.minimum[axis] - AABB_MARGIN;
					if (separatedIn3D)
						continue;
					if (left.maximumU < right.minimumU - AABB_MARGIN
						|| right.maximumU < left.minimumU - AABB_MARGIN
						|| left.maximumV < right.minimumV - AABB_MARGIN
						|| right.maximumV < left.minimumV - AABB_MARGIN)
						continue;
					if (!projectedTrianglesOverlap(left, right))
						continue;
					const auto& leftTag = taggedMesh.tags[left.triangleIndex];
					const auto& rightTag = taggedMesh.tags[right.triangleIndex];
					std::ostringstream message;
					message << "Spaceship render mesh has near-coplanar overlapping faces: left="
						<< left.triangleIndex << " surface=" << static_cast<int>(leftTag.surface)
						<< " owner=" << leftTag.mountOwner << " right=" << right.triangleIndex
						<< " surface=" << static_cast<int>(rightTag.surface)
						<< " owner=" << rightTag.mountOwner
						<< " planeDistance=" << std::abs(right.planeOffset - left.planeOffset)
						<< " normalDot=" << gen_model::gen_types::dot(left.normal, right.normal);
					throw std::invalid_argument(message.str());
				}
			}
		}
	}

	void weldCoincidentPositionsAndRemoveDuplicateFaces(
		gen_model::spaceship::detail::TaggedMesh& taggedMesh
	) {
		if (taggedMesh.mesh.triangles.empty())
			return;

		std::unordered_map<WeldPositionKey, int, WeldPositionKeyHash> canonicalPositions;
		canonicalPositions.reserve(taggedMesh.mesh.positions.size());
		std::vector<int> positionRemap(taggedMesh.mesh.positions.size(), -1);
		std::vector<gen_model::gen_types::Point3> weldedPositions;
		weldedPositions.reserve(taggedMesh.mesh.positions.size());
		for (std::size_t index = 0; index < taggedMesh.mesh.positions.size(); ++index) {
			const WeldPositionKey key = weldPositionKey(taggedMesh.mesh.positions[index]);
			const auto [iterator, inserted] = canonicalPositions.emplace(
				key,
				static_cast<int>(weldedPositions.size())
			);
			if (inserted)
				weldedPositions.push_back(taggedMesh.mesh.positions[index]);
			positionRemap[index] = iterator->second;
		}

		std::vector<gen_model::gen_types::Triangle> triangles;
		std::vector<gen_model::spaceship::detail::TriangleTag> tags;
		triangles.reserve(taggedMesh.mesh.triangles.size());
		tags.reserve(taggedMesh.tags.size());
		std::unordered_set<FaceKey, FaceKeyHash> faces;
		faces.reserve(taggedMesh.mesh.triangles.size());
		for (std::size_t triangleIndex = 0; triangleIndex < taggedMesh.mesh.triangles.size(); ++triangleIndex) {
			gen_model::gen_types::Triangle triangle = taggedMesh.mesh.triangles[triangleIndex];
			for (int& positionIndex : triangle.positionIndices) {
				if (positionIndex < 0
					|| static_cast<std::size_t>(positionIndex) >= positionRemap.size())
					throw std::invalid_argument("Spaceship mesh position index is out of range while welding");
				positionIndex = positionRemap[static_cast<std::size_t>(positionIndex)];
			}
			if (triangle.positionIndices[0] == triangle.positionIndices[1]
				|| triangle.positionIndices[1] == triangle.positionIndices[2]
				|| triangle.positionIndices[2] == triangle.positionIndices[0])
				throw std::invalid_argument("Spaceship mesh welding collapsed a triangle");
			std::array<int, 3> sortedPositions = triangle.positionIndices;
			std::sort(sortedPositions.begin(), sortedPositions.end());
			if (!faces.emplace(FaceKey{sortedPositions}).second)
				continue;
			triangles.push_back(triangle);
			if (triangleIndex >= taggedMesh.tags.size())
				throw std::logic_error("Spaceship triangle tags are out of sync while welding");
			tags.push_back(taggedMesh.tags[triangleIndex]);
		}

		if (triangles.empty())
			throw std::invalid_argument("Spaceship mesh welding removed every triangle");
		taggedMesh.mesh.positions = std::move(weldedPositions);
		taggedMesh.mesh.triangles = std::move(triangles);
		taggedMesh.tags = std::move(tags);
	}

	PositionEdgeKey positionEdgeKey(int left, int right) {
		if (left > right)
			std::swap(left, right);
		return {left, right};
	}

	std::vector<TriangleComponent> buildTriangleComponents(
		const gen_model::spaceship::detail::TaggedMesh& taggedMesh
	) {
		const auto& triangles = taggedMesh.mesh.triangles;
		std::vector<std::vector<std::size_t>> adjacency(triangles.size());
		std::unordered_map<PositionEdgeKey, std::size_t, PositionEdgeKeyHash> edgeOwners;
		edgeOwners.reserve(triangles.size() * 2u);
		std::unordered_set<PositionEdgeKey, PositionEdgeKeyHash> boundaryEdges;
		for (std::size_t triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex) {
			const auto& triangle = triangles[triangleIndex];
			for (std::size_t corner = 0; corner < 3u; ++corner) {
				const std::size_t next = (corner + 1u) % 3u;
				const PositionEdgeKey edge = positionEdgeKey(
					triangle.positionIndices[corner],
					triangle.positionIndices[next]
				);
				const auto [owner, inserted] = edgeOwners.emplace(edge, triangleIndex);
				if (inserted)
					continue;
				if (owner->second == triangleIndex)
					continue;
				adjacency[triangleIndex].push_back(owner->second);
				adjacency[owner->second].push_back(triangleIndex);
				edgeOwners.erase(owner);
			}
		}
		for (const auto& [edge, owner] : edgeOwners) {
			(void)owner;
			boundaryEdges.insert(edge);
		}

		std::vector<TriangleComponent> components;
		std::vector<bool> visited(triangles.size(), false);
		std::vector<std::size_t> pending;
		for (std::size_t start = 0; start < triangles.size(); ++start) {
			if (visited[start])
				continue;
			TriangleComponent component;
			pending.push_back(start);
			visited[start] = true;
			while (!pending.empty()) {
				const std::size_t triangleIndex = pending.back();
				pending.pop_back();
				component.triangles.push_back(triangleIndex);
				const auto& triangle = triangles[triangleIndex];
				for (const int positionIndex : triangle.positionIndices)
					component.bounds.include(
						taggedMesh.mesh.positions[static_cast<std::size_t>(positionIndex)]
					);
				for (const std::size_t neighbor : adjacency[triangleIndex]) {
					if (visited[neighbor])
						continue;
					visited[neighbor] = true;
					pending.push_back(neighbor);
				}
			}
			for (const std::size_t triangleIndex : component.triangles) {
				const auto& triangle = triangles[triangleIndex];
				for (std::size_t corner = 0; corner < 3u; ++corner) {
					const std::size_t next = (corner + 1u) % 3u;
					if (boundaryEdges.contains(positionEdgeKey(
						triangle.positionIndices[corner],
						triangle.positionIndices[next]
					)))
						component.closed = false;
				}
			}
			components.push_back(std::move(component));
		}
		return components;
	}

	bool rayIntersectsTriangle(
		gen_model::gen_types::Point3 origin,
		gen_model::gen_types::Point3 direction,
		gen_model::gen_types::Point3 a,
		gen_model::gen_types::Point3 b,
		gen_model::gen_types::Point3 c
	) {
		constexpr float EPSILON = 0.0000001f;
		const auto edge1 = b - a;
		const auto edge2 = c - a;
		const auto crossDirection = gen_model::gen_types::cross(direction, edge2);
		const float determinant = gen_model::gen_types::dot(edge1, crossDirection);
		if (std::abs(determinant) <= EPSILON)
			return false;
		const float inverseDeterminant = 1.0f / determinant;
		const auto offset = origin - a;
		const float u = gen_model::gen_types::dot(offset, crossDirection) * inverseDeterminant;
		if (u <= EPSILON || u >= 1.0f - EPSILON)
			return false;
		const auto crossOffset = gen_model::gen_types::cross(offset, edge1);
		const float v = gen_model::gen_types::dot(direction, crossOffset) * inverseDeterminant;
		if (v <= EPSILON || u + v >= 1.0f - EPSILON)
			return false;
		const float distance = gen_model::gen_types::dot(edge2, crossOffset) * inverseDeterminant;
		return distance > EPSILON;
	}

	bool pointInsideComponent(
		gen_model::gen_types::Point3 point,
		const TriangleComponent& component,
		const gen_model::spaceship::detail::TaggedMesh& taggedMesh
	) {
		if (!component.closed || !component.bounds.contains(point, 0.0001f))
			return false;
		const auto direction = gen_model::gen_types::normalize(
			gen_model::gen_types::Point3{1.0f, 0.371f, 0.157f}
		);
		std::size_t intersections = 0u;
		for (const std::size_t triangleIndex : component.triangles) {
			const auto& triangle = taggedMesh.mesh.triangles[triangleIndex];
			if (rayIntersectsTriangle(
				point,
				direction,
				taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])],
				taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])],
				taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])]
			))
				++intersections;
		}
		return (intersections % 2u) == 1u;
	}

	void compactMesh(
		gen_model::spaceship::detail::TaggedMesh& taggedMesh,
		const std::vector<bool>& keepTriangle
	) {
		using MeshData = gen_model::gen_types::MeshData;
		MeshData compacted;
		std::vector<gen_model::spaceship::detail::TriangleTag> tags;
		std::vector<int> positionRemap(taggedMesh.mesh.positions.size(), -1);
		std::vector<int> texcoordRemap(taggedMesh.mesh.texcoords.size(), -1);
		std::vector<int> normalRemap(taggedMesh.mesh.normals.size(), -1);
		const auto remapIndex = [](int index, auto& source, auto& target, auto& remap) {
			if (index < 0 || static_cast<std::size_t>(index) >= source.size())
				throw std::invalid_argument("Spaceship mesh index is out of range while compacting");
			int& mapped = remap[static_cast<std::size_t>(index)];
			if (mapped >= 0)
				return mapped;
			mapped = static_cast<int>(target.size());
			target.push_back(source[static_cast<std::size_t>(index)]);
			return mapped;
		};
		for (std::size_t triangleIndex = 0; triangleIndex < taggedMesh.mesh.triangles.size(); ++triangleIndex) {
			if (!keepTriangle[triangleIndex])
				continue;
			const auto& triangle = taggedMesh.mesh.triangles[triangleIndex];
			gen_model::gen_types::Triangle compactTriangle = triangle;
			for (int& index : compactTriangle.positionIndices)
				index = remapIndex(index, taggedMesh.mesh.positions, compacted.positions, positionRemap);
			for (int& index : compactTriangle.texcoordIndices)
				index = remapIndex(index, taggedMesh.mesh.texcoords, compacted.texcoords, texcoordRemap);
			for (int& index : compactTriangle.normalIndices)
				index = remapIndex(index, taggedMesh.mesh.normals, compacted.normals, normalRemap);
			compacted.triangles.push_back(compactTriangle);
			if (triangleIndex >= taggedMesh.tags.size())
				throw std::logic_error("Spaceship triangle tags are out of sync while compacting");
			tags.push_back(taggedMesh.tags[triangleIndex]);
		}
		if (compacted.triangles.empty())
			throw std::invalid_argument("Spaceship mesh cleanup removed every triangle");
		taggedMesh.mesh = std::move(compacted);
		taggedMesh.tags = std::move(tags);
	}

	void removeBuriedRenderFaces(
		gen_model::spaceship::detail::TaggedMesh& taggedMesh
	) {
		if (taggedMesh.mesh.triangles.empty())
			return;
		const auto components = buildTriangleComponents(taggedMesh);
		if (components.size() < 2u)
			return;
		std::vector<bool> keepTriangle(taggedMesh.mesh.triangles.size(), true);
		// Remove only complete buried shells.  Deleting a subset of triangles from
		// an intersecting shell would create a hole unless a boolean splitter also
		// emitted the new intersection ring; exposed mechanical islands stay intact.
		for (std::size_t componentIndex = 0; componentIndex < components.size(); ++componentIndex) {
			const auto& component = components[componentIndex];
			if (!component.closed)
				continue;
			const float componentVolume = component.bounds.volume();
			const auto componentCenter = (component.bounds.minimum + component.bounds.maximum) * 0.5f;
			bool fullyBuried = false;
			for (std::size_t otherIndex = 0; otherIndex < components.size(); ++otherIndex) {
				if (otherIndex == componentIndex)
					continue;
				const auto& other = components[otherIndex];
				if (!other.closed
					|| other.bounds.volume() <= componentVolume * 1.0001f
					|| !other.bounds.contains(component.bounds, 0.0001f))
					continue;
				if (pointInsideComponent(componentCenter, other, taggedMesh)) {
					fullyBuried = true;
					break;
				}
			}
			if (!fullyBuried)
				continue;
			for (const std::size_t triangleIndex : component.triangles)
				keepTriangle[triangleIndex] = false;
		}
		if (std::all_of(keepTriangle.begin(), keepTriangle.end(), [](bool keep) { return keep; }))
		{
			return;
		}
		compactMesh(taggedMesh, keepTriangle);
	}

	SmoothNormalKey smoothNormalKey(
		const gen_model::gen_types::Point3& point,
		gen_model::spaceship::detail::Surface surface
	) {
		constexpr float QUANTIZATION = 100000.0f;
		return {
			static_cast<std::int64_t>(std::llround(point.x * QUANTIZATION)),
			static_cast<std::int64_t>(std::llround(point.y * QUANTIZATION)),
			static_cast<std::int64_t>(std::llround(point.z * QUANTIZATION)),
			static_cast<int>(surface)
		};
	}
}

void gen_model::spaceship::detail::MeshBuilder::addTriangle(
	gen_model::gen_types::Point3 a,
	gen_model::gen_types::Point3 b,
	gen_model::gen_types::Point3 c,
	gen_model::gen_types::Point3 expectedNormal,
	gen_model::spaceship::detail::Surface surface,
	int mountOwner,
	bool socket
) {
	gen_model::gen_types::Point3 normal = gen_model::gen_types::cross(b - a, c - a);
	if (gen_model::gen_types::dot(normal, expectedNormal) < 0.0f) {
		std::swap(b, c);
		normal = normal * -1.0f;
	}
	if (gen_model::gen_types::dot(normal, normal) <= 0.00000001f) {
		std::ostringstream message;
		message << "Cannot append a degenerate spaceship triangle: ("
			<< a.x << ',' << a.y << ',' << a.z << ") ("
			<< b.x << ',' << b.y << ',' << b.z << ") ("
			<< c.x << ',' << c.y << ',' << c.z << ')';
		throw std::invalid_argument(message.str());
	}
	normal = gen_model::gen_types::normalize(normal);
	const int base = static_cast<int>(result.mesh.positions.size());
	result.mesh.positions.insert(result.mesh.positions.end(), {a, b, c});
	result.mesh.normals.insert(result.mesh.normals.end(), {normal, normal, normal});
	const auto uvs = surface == Surface::Armor
		? std::array<gen_model::gen_types::Point2, 3>{
			continuousArmorUv(a), continuousArmorUv(b), continuousArmorUv(c)
		}
		: triangleUvs(surface);
	result.mesh.texcoords.insert(result.mesh.texcoords.end(), uvs.begin(), uvs.end());
	result.mesh.triangles.push_back({{base, base + 1, base + 2}, {base, base + 1, base + 2}, {base, base + 1, base + 2}});
	result.tags.push_back({surface, mountOwner, socket});
}

void gen_model::spaceship::detail::MeshBuilder::addQuad(
	gen_model::gen_types::Point3 a,
	gen_model::gen_types::Point3 b,
	gen_model::gen_types::Point3 c,
	gen_model::gen_types::Point3 d,
	gen_model::gen_types::Point3 expectedNormal,
	gen_model::spaceship::detail::Surface surface,
	int mountOwner,
	bool socket
) {
	std::array<gen_model::gen_types::Point3, 4> vertices{a, b, c, d};
	gen_model::gen_types::Point3 normal = gen_model::gen_types::cross(b - a, c - a);
	if (gen_model::gen_types::dot(normal, expectedNormal) < 0.0f) {
		std::swap(vertices[1], vertices[3]);
		normal = normal * -1.0f;
	}
	if (gen_model::gen_types::dot(normal, normal) <= 0.00000001f) {
		std::ostringstream message;
		message << "Cannot append a degenerate spaceship quad: ("
			<< a.x << ',' << a.y << ',' << a.z << ") ("
			<< b.x << ',' << b.y << ',' << b.z << ") ("
			<< c.x << ',' << c.y << ',' << c.z << ") ("
			<< d.x << ',' << d.y << ',' << d.z << ')';
		throw std::invalid_argument(message.str());
	}
	normal = gen_model::gen_types::normalize(normal);
	const auto uvs = surface == Surface::Armor
		? std::array<gen_model::gen_types::Point2, 4>{
			continuousArmorUv(vertices[0]), continuousArmorUv(vertices[1]),
			continuousArmorUv(vertices[2]), continuousArmorUv(vertices[3])
		}
		: quadUvs(surface);
	const int base = static_cast<int>(result.mesh.positions.size());
	for (std::size_t index = 0; index < vertices.size(); ++index) {
		result.mesh.positions.push_back(vertices[index]);
		result.mesh.normals.push_back(normal);
		result.mesh.texcoords.push_back(uvs[index]);
	}
	result.mesh.triangles.push_back({{base, base + 1, base + 2}, {base, base + 1, base + 2}, {base, base + 1, base + 2}});
	result.mesh.triangles.push_back({{base, base + 2, base + 3}, {base, base + 2, base + 3}, {base, base + 2, base + 3}});
	result.tags.push_back({surface, mountOwner, socket});
	result.tags.push_back({surface, mountOwner, socket});
}

void gen_model::spaceship::detail::MeshBuilder::addLoftSides(
	const std::vector<std::vector<gen_model::gen_types::Point3>>& rings,
	gen_model::spaceship::detail::Surface surface,
	int mountOwner,
	bool socket
) {
	if (rings.size() < 2)
		throw std::invalid_argument("A spaceship loft needs at least two rings");
	if (rings.front().size() < 3)
		throw std::invalid_argument("A spaceship loft ring needs at least three points");
	const std::size_t ringSize = rings.front().size();
	for (const auto& ring : rings) {
		if (ring.size() != ringSize)
			throw std::invalid_argument("Spaceship loft rings must have equal point counts");
	}

	std::vector<gen_model::gen_types::Point3> centers;
	centers.reserve(rings.size());
	for (const auto& ring : rings) {
		gen_model::gen_types::Point3 center{};
		for (const auto point : ring)
			center = center + point;
		centers.push_back(divide(center, static_cast<float>(ring.size())));
	}

	float strongestAlignment = 0.0f;
	bool reverseWinding = false;
	for (std::size_t station = 0; station + 1 < rings.size(); ++station) {
		const auto centerLine = midpoint(centers[station], centers[station + 1]);
		for (std::size_t section = 0; section < ringSize; ++section) {
			const std::size_t next = (section + 1u) % ringSize;
			const auto& a = rings[station][section];
			const auto& b = rings[station + 1][section];
			const auto& c = rings[station + 1][next];
			const auto& d = rings[station][next];
			const auto normal = gen_model::gen_types::cross(b - a, c - a);
			const auto faceCenter = (a + b + c + d) * 0.25f;
			const float alignment = gen_model::gen_types::dot(normal, faceCenter - centerLine);
			if (std::abs(alignment) <= strongestAlignment)
				continue;
			strongestAlignment = std::abs(alignment);
			reverseWinding = alignment < 0.0f;
		}
	}
	if (strongestAlignment <= 0.00000001f)
		throw std::invalid_argument("Cannot determine spaceship loft orientation");

	for (std::size_t station = 0; station + 1 < rings.size(); ++station) {
		for (std::size_t section = 0; section < ringSize; ++section) {
			const std::size_t next = (section + 1u) % ringSize;
			const auto& a = rings[station][section];
			const auto& b = rings[station + 1][section];
			const auto& c = rings[station + 1][next];
			const auto& d = rings[station][next];
			if (reverseWinding) {
				addQuad(
					a, d, c, b,
					gen_model::gen_types::cross(d - a, c - a),
					surface, mountOwner, socket
				);
				continue;
			}
			addQuad(
				a, b, c, d,
				gen_model::gen_types::cross(b - a, c - a),
				surface, mountOwner, socket
			);
		}
	}
}

void gen_model::spaceship::detail::MeshBuilder::addConvexCap(
	const std::vector<gen_model::gen_types::Point3>& ring,
	gen_model::gen_types::Point3 expectedNormal,
	gen_model::spaceship::detail::Surface surface,
	int mountOwner,
	bool socket
) {
	if (ring.size() < 3)
		throw std::invalid_argument("A spaceship cap needs at least three points");
	if (gen_model::gen_types::dot(expectedNormal, expectedNormal) <= 0.00000001f)
		throw std::invalid_argument("A spaceship cap needs a non-zero outward normal");

	gen_model::gen_types::Point3 center{};
	for (const auto point : ring)
		center = center + point;
	center = divide(center, static_cast<float>(ring.size()));
	for (std::size_t section = 0; section < ring.size(); ++section) {
		const std::size_t next = (section + 1u) % ring.size();
		addTriangle(
			center,
			ring[section],
			ring[next],
			expectedNormal,
			surface,
			mountOwner,
			socket
		);
	}
}

void gen_model::spaceship::detail::MeshBuilder::addClosedLoft(
	const std::vector<std::vector<gen_model::gen_types::Point3>>& rings,
	gen_model::spaceship::detail::Surface sideSurface,
	gen_model::spaceship::detail::Surface startCapSurface,
	gen_model::spaceship::detail::Surface endCapSurface,
	int mountOwner,
	bool socket
) {
	addLoftSides(rings, sideSurface, mountOwner, socket);

	gen_model::gen_types::Point3 startCenter{};
	gen_model::gen_types::Point3 nextCenter{};
	gen_model::gen_types::Point3 endCenter{};
	gen_model::gen_types::Point3 previousCenter{};
	for (const auto point : rings.front())
		startCenter = startCenter + point;
	for (const auto point : rings[1])
		nextCenter = nextCenter + point;
	for (const auto point : rings.back())
		endCenter = endCenter + point;
	for (const auto point : rings[rings.size() - 2u])
		previousCenter = previousCenter + point;
	startCenter = divide(startCenter, static_cast<float>(rings.front().size()));
	nextCenter = divide(nextCenter, static_cast<float>(rings[1].size()));
	endCenter = divide(endCenter, static_cast<float>(rings.back().size()));
	previousCenter = divide(previousCenter, static_cast<float>(rings[rings.size() - 2u].size()));

	addConvexCap(
		rings.front(),
		startCenter - nextCenter,
		startCapSurface,
		mountOwner,
		socket
	);
	addConvexCap(
		rings.back(),
		endCenter - previousCenter,
		endCapSurface,
		mountOwner,
		socket
	);
}

void gen_model::spaceship::detail::MeshBuilder::addBox(
	gen_model::gen_types::Point3 center,
	gen_model::gen_types::Point3 size,
	gen_model::spaceship::detail::Surface surface
) {
	const gen_model::gen_types::Point3 half = size * 0.5f;
	const gen_model::gen_types::Point3 minimum = center - half;
	const gen_model::gen_types::Point3 maximum = center + half;
	const gen_model::gen_types::Point3 p000{minimum.x, minimum.y, minimum.z};
	const gen_model::gen_types::Point3 p001{minimum.x, minimum.y, maximum.z};
	const gen_model::gen_types::Point3 p010{minimum.x, maximum.y, minimum.z};
	const gen_model::gen_types::Point3 p011{minimum.x, maximum.y, maximum.z};
	const gen_model::gen_types::Point3 p100{maximum.x, minimum.y, minimum.z};
	const gen_model::gen_types::Point3 p101{maximum.x, minimum.y, maximum.z};
	const gen_model::gen_types::Point3 p110{maximum.x, maximum.y, minimum.z};
	const gen_model::gen_types::Point3 p111{maximum.x, maximum.y, maximum.z};
	addQuad(p100, p101, p111, p110, {1.0f, 0.0f, 0.0f}, surface);
	addQuad(p001, p000, p010, p011, {-1.0f, 0.0f, 0.0f}, surface);
	addQuad(p010, p110, p111, p011, {0.0f, 1.0f, 0.0f}, surface);
	addQuad(p000, p001, p101, p100, {0.0f, -1.0f, 0.0f}, surface);
	addQuad(p001, p011, p111, p101, {0.0f, 0.0f, 1.0f}, surface);
	addQuad(p000, p100, p110, p010, {0.0f, 0.0f, -1.0f}, surface);
}

void gen_model::spaceship::detail::MeshBuilder::addTaperedBeam(
	gen_model::gen_types::Point3 start,
	gen_model::gen_types::Point3 end,
	float startWidth,
	float endWidth,
	float height,
	gen_model::spaceship::detail::Surface surface,
	int mountOwner
) {
	const gen_model::gen_types::Point3 axis = gen_model::gen_types::normalize(end - start);
	gen_model::gen_types::Point3 referenceUp{0.0f, 1.0f, 0.0f};
	if (std::abs(gen_model::gen_types::dot(axis, referenceUp)) > 0.95f)
		referenceUp = {0.0f, 0.0f, 1.0f};
	const gen_model::gen_types::Point3 right = gen_model::gen_types::normalize(gen_model::gen_types::cross(referenceUp, axis));
	const gen_model::gen_types::Point3 up = gen_model::gen_types::normalize(gen_model::gen_types::cross(axis, right));
	const gen_model::gen_types::Point3 startRight = right * (startWidth * 0.5f);
	const gen_model::gen_types::Point3 endRight = right * (endWidth * 0.5f);
	const gen_model::gen_types::Point3 vertical = up * (height * 0.5f);
	const std::array<gen_model::gen_types::Point3, 4> startRing{
		start - startRight - vertical,
		start + startRight - vertical,
		start + startRight + vertical,
		start - startRight + vertical
	};
	const std::array<gen_model::gen_types::Point3, 4> endRing{
		end - endRight - vertical,
		end + endRight - vertical,
		end + endRight + vertical,
		end - endRight + vertical
	};
	for (int side = 0; side < 4; ++side) {
		const int next = (side + 1) % 4;
		const gen_model::gen_types::Point3 faceCenter = (
			startRing[side] + startRing[next] + endRing[next] + endRing[side]
		) * 0.25f;
		const gen_model::gen_types::Point3 centerLine = midpoint(start, end);
		addQuad(
			startRing[side], startRing[next], endRing[next], endRing[side],
			faceCenter - centerLine, surface, mountOwner
		);
	}
	addQuad(startRing[0], startRing[3], startRing[2], startRing[1], axis * -1.0f, surface, mountOwner);
	addQuad(endRing[0], endRing[1], endRing[2], endRing[3], axis, surface, mountOwner);
}

void gen_model::spaceship::detail::MeshBuilder::addPrismY(
	const std::vector<gen_model::spaceship::detail::PlanPoint>& plan,
	float bottomY,
	float topY,
	gen_model::spaceship::detail::Surface surface
) {
	if (plan.size() < 3)
		throw std::invalid_argument("A spaceship prism needs at least three plan points");
	gen_model::gen_types::Point3 center{};
	for (const auto point : plan)
		center = center + gen_model::gen_types::Point3{point.x, (bottomY + topY) * 0.5f, point.z};
	center = divide(center, static_cast<float>(plan.size()));
	for (std::size_t index = 1; index + 1 < plan.size(); ++index) {
		addTriangle(
			{plan[0].x, topY, plan[0].z},
			{plan[index].x, topY, plan[index].z},
			{plan[index + 1].x, topY, plan[index + 1].z},
			{0.0f, 1.0f, 0.0f}, surface
		);
		addTriangle(
			{plan[0].x, bottomY, plan[0].z},
			{plan[index + 1].x, bottomY, plan[index + 1].z},
			{plan[index].x, bottomY, plan[index].z},
			{0.0f, -1.0f, 0.0f}, surface
		);
	}
	for (std::size_t index = 0; index < plan.size(); ++index) {
		const std::size_t next = (index + 1) % plan.size();
		const gen_model::gen_types::Point3 edgeCenter{
			(plan[index].x + plan[next].x) * 0.5f,
			(bottomY + topY) * 0.5f,
			(plan[index].z + plan[next].z) * 0.5f
		};
		addQuad(
			{plan[index].x, bottomY, plan[index].z},
			{plan[next].x, bottomY, plan[next].z},
			{plan[next].x, topY, plan[next].z},
			{plan[index].x, topY, plan[index].z},
			edgeCenter - center, surface
		);
	}
}

void gen_model::spaceship::detail::MeshBuilder::addCylinderY(
	gen_model::gen_types::Point3 center,
	float radius,
	float height,
	int segments,
	gen_model::spaceship::detail::Surface surface,
	int mountOwner,
	bool socket
) {
	validateSegments(segments);
	const float bottom = center.y - height * 0.5f;
	const float top = center.y + height * 0.5f;
	for (int segment = 0; segment < segments; ++segment) {
		const float angleA = 2.0f * PI * static_cast<float>(segment) / static_cast<float>(segments);
		const float angleB = 2.0f * PI * static_cast<float>(segment + 1) / static_cast<float>(segments);
		const gen_model::gen_types::Point3 radialA{std::cos(angleA), 0.0f, std::sin(angleA)};
		const gen_model::gen_types::Point3 radialB{std::cos(angleB), 0.0f, std::sin(angleB)};
		const gen_model::gen_types::Point3 aBottom = center + radialA * radius + gen_model::gen_types::Point3{0.0f, bottom - center.y, 0.0f};
		const gen_model::gen_types::Point3 bBottom = center + radialB * radius + gen_model::gen_types::Point3{0.0f, bottom - center.y, 0.0f};
		const gen_model::gen_types::Point3 aTop = center + radialA * radius + gen_model::gen_types::Point3{0.0f, top - center.y, 0.0f};
		const gen_model::gen_types::Point3 bTop = center + radialB * radius + gen_model::gen_types::Point3{0.0f, top - center.y, 0.0f};
		addQuad(aBottom, bBottom, bTop, aTop, radialA + radialB, surface, mountOwner, socket);
		addTriangle({center.x, top, center.z}, aTop, bTop, {0.0f, 1.0f, 0.0f}, surface, mountOwner, socket);
		addTriangle({center.x, bottom, center.z}, bBottom, aBottom, {0.0f, -1.0f, 0.0f}, surface, mountOwner, socket);
	}
}

void gen_model::spaceship::detail::MeshBuilder::addCylinderZ(
	gen_model::gen_types::Point3 center,
	float radius,
	float length,
	int segments,
	gen_model::spaceship::detail::Surface surface
) {
	validateSegments(segments);
	const float rear = center.z - length * 0.5f;
	const float front = center.z + length * 0.5f;
	for (int segment = 0; segment < segments; ++segment) {
		const float angleA = 2.0f * PI * static_cast<float>(segment) / static_cast<float>(segments);
		const float angleB = 2.0f * PI * static_cast<float>(segment + 1) / static_cast<float>(segments);
		const gen_model::gen_types::Point3 radialA{std::cos(angleA), std::sin(angleA), 0.0f};
		const gen_model::gen_types::Point3 radialB{std::cos(angleB), std::sin(angleB), 0.0f};
		const gen_model::gen_types::Point3 aRear = center + radialA * radius + gen_model::gen_types::Point3{0.0f, 0.0f, rear - center.z};
		const gen_model::gen_types::Point3 bRear = center + radialB * radius + gen_model::gen_types::Point3{0.0f, 0.0f, rear - center.z};
		const gen_model::gen_types::Point3 aFront = center + radialA * radius + gen_model::gen_types::Point3{0.0f, 0.0f, front - center.z};
		const gen_model::gen_types::Point3 bFront = center + radialB * radius + gen_model::gen_types::Point3{0.0f, 0.0f, front - center.z};
		addQuad(aRear, aFront, bFront, bRear, radialA + radialB, surface);
		addTriangle({center.x, center.y, front}, aFront, bFront, {0.0f, 0.0f, 1.0f}, surface);
		addTriangle({center.x, center.y, rear}, bRear, aRear, {0.0f, 0.0f, -1.0f}, surface);
	}
}

std::optional<float>
gen_model::spaceship::detail::MeshBuilder::topStructuralSurfaceY(float x, float z) const {
	std::optional<float> resultY;
	for (std::size_t triangleIndex = 0; triangleIndex < result.mesh.triangles.size(); ++triangleIndex) {
		const Surface surface = result.tags[triangleIndex].surface;
		if (surface != Surface::Armor && surface != Surface::Structure)
			continue;
		const auto& triangle = result.mesh.triangles[triangleIndex];
		const auto& a = result.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])];
		const auto& b = result.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])];
		const auto& c = result.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])];
		const float denominator = (b.z - c.z) * (a.x - c.x)
			+ (c.x - b.x) * (a.z - c.z);
		if (std::abs(denominator) <= 0.0000001f)
			continue;
		const float weightA = ((b.z - c.z) * (x - c.x)
			+ (c.x - b.x) * (z - c.z)) / denominator;
		const float weightB = ((c.z - a.z) * (x - c.x)
			+ (a.x - c.x) * (z - c.z)) / denominator;
		const float weightC = 1.0f - weightA - weightB;
		constexpr float BARYCENTRIC_TOLERANCE = 0.00001f;
		if (weightA < -BARYCENTRIC_TOLERANCE
			|| weightB < -BARYCENTRIC_TOLERANCE
			|| weightC < -BARYCENTRIC_TOLERANCE)
			continue;
		const float y = weightA * a.y + weightB * b.y + weightC * c.y;
		if (!resultY.has_value() || y > *resultY)
			resultY = y;
	}
	return resultY;
}

gen_model::spaceship::detail::TaggedMesh gen_model::spaceship::detail::MeshBuilder::finish() {
	if (result.mesh.triangles.size() != result.tags.size())
		throw std::logic_error("Spaceship triangle tags are out of sync");
	// Keep one exterior render surface per geometric face while preserving
	// deliberate material and normal seams in their separate attribute streams.
	weldCoincidentPositionsAndRemoveDuplicateFaces(result);
	// Coplanar component conflicts are resolved at their source (socket/engine
	// spacing) before export.  A generic post-build offset would silently move
	// structural parts away from their attachment points, so keep the exporter
	// geometrically faithful here.
	removeBuriedRenderFaces(result);
	requireNoCoplanarRenderOverlaps(result);
	requireNoNearCoplanarRenderOverlaps(result);
	std::unordered_map<SmoothNormalKey, std::vector<gen_model::gen_types::Point3>, SmoothNormalKeyHash> normalOccurrences;
	for (std::size_t triangleIndex = 0; triangleIndex < result.mesh.triangles.size(); ++triangleIndex) {
		const TriangleTag& tag = result.tags[triangleIndex];
		if (!isSmoothSurface(tag.surface))
			continue;
		const auto& triangle = result.mesh.triangles[triangleIndex];
		for (const int positionIndex : triangle.positionIndices) {
			const auto key = smoothNormalKey(
				result.mesh.positions[static_cast<std::size_t>(positionIndex)],
				tag.surface
			);
			normalOccurrences[key].push_back(result.mesh.normals[static_cast<std::size_t>(positionIndex)]);
		}
	}
	// A 0.35 cosine gate lets adjacent pressure-skin and airfoil faces share a
	// continuous highlight while still preserving genuine breaks at the wing edge,
	// socket, and separate mechanical hardware.  The former 0.80 gate left every
	// low-poly loft strip visibly faceted in the close three-quarter QA view.
	constexpr float SMOOTH_NORMAL_DOT = 0.35f;
	for (std::size_t triangleIndex = 0; triangleIndex < result.mesh.triangles.size(); ++triangleIndex) {
		const TriangleTag& tag = result.tags[triangleIndex];
		if (!isSmoothSurface(tag.surface))
			continue;
		const auto& triangle = result.mesh.triangles[triangleIndex];
		for (const int positionIndex : triangle.positionIndices) {
			const auto key = smoothNormalKey(
				result.mesh.positions[static_cast<std::size_t>(positionIndex)],
				tag.surface
			);
			const auto& originalNormal = result.mesh.normals[static_cast<std::size_t>(positionIndex)];
			gen_model::gen_types::Point3 clusterSum{};
			for (const auto& candidate : normalOccurrences.at(key)) {
				if (gen_model::gen_types::dot(originalNormal, candidate) >= SMOOTH_NORMAL_DOT)
					clusterSum = clusterSum + candidate;
			}
			const auto normal = gen_model::gen_types::normalize(clusterSum);
			result.mesh.normals[static_cast<std::size_t>(positionIndex)] = normal;
		}
	}
	return std::move(result);
}
