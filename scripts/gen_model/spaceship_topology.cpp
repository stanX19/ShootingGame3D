#include "spaceship_topology.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {
	using namespace gen_model::gen_types;
	struct PositionKey {
		std::int64_t x;
		std::int64_t y;
		std::int64_t z;

		bool operator==(const PositionKey&) const = default;
	};

	bool keyLess(const PositionKey& left, const PositionKey& right) {
		if (left.x != right.x)
			return left.x < right.x;
		if (left.y != right.y)
			return left.y < right.y;
		return left.z < right.z;
	}

	struct EdgeKey {
		PositionKey start;
		PositionKey end;

		bool operator==(const EdgeKey&) const = default;
	};

	std::size_t combineHash(std::size_t seed, std::int64_t value) {
		const std::size_t hashed = std::hash<std::int64_t>{}(value);
		return seed ^ (hashed + 0x9E3779B9u + (seed << 6u) + (seed >> 2u));
	}

	struct PositionKeyHash {
		std::size_t operator()(const PositionKey& key) const noexcept {
			std::size_t result = 0;
			result = combineHash(result, key.x);
			result = combineHash(result, key.y);
			return combineHash(result, key.z);
		}
	};

	struct EdgeKeyHash {
		std::size_t operator()(const EdgeKey& key) const noexcept {
			std::size_t result = 0;
			result = combineHash(result, key.start.x);
			result = combineHash(result, key.start.y);
			result = combineHash(result, key.start.z);
			result = combineHash(result, key.end.x);
			result = combineHash(result, key.end.y);
			return combineHash(result, key.end.z);
		}
	};

	struct EdgeUse {
		gen_model::gen_types::Point3 start{};
		gen_model::gen_types::Point3 end{};
		std::size_t uses = 0;
		int orientationBalance = 0;
		std::size_t firstTriangle = 0;
		std::array<std::size_t, 2> triangles{};
	};

	bool finite(gen_model::gen_types::Point3 point) {
		return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
	}

	PositionKey positionKey(
		gen_model::gen_types::Point3 point,
		float weldTolerance
	) {
		const double scale = 1.0 / static_cast<double>(weldTolerance);
		return {
			static_cast<std::int64_t>(std::llround(static_cast<double>(point.x) * scale)),
			static_cast<std::int64_t>(std::llround(static_cast<double>(point.y) * scale)),
			static_cast<std::int64_t>(std::llround(static_cast<double>(point.z) * scale))
		};
	}

	template <std::size_t Size>
	bool indicesInside(const std::array<int, Size>& indices, std::size_t count) {
		return std::all_of(
			indices.begin(),
			indices.end(),
			[count](int index) {
				return index >= 0 && static_cast<std::size_t>(index) < count;
			}
		);
	}

	const char* issueName(gen_model::spaceship::topology::IssueKind kind) {
		using IssueKind = gen_model::spaceship::topology::IssueKind;
		switch (kind) {
			case IssueKind::InvalidTriangle:
				return "invalid triangle";
			case IssueKind::DegenerateTriangle:
				return "degenerate triangle";
			case IssueKind::BoundaryEdge:
				return "boundary edge";
			case IssueKind::NonManifoldEdge:
				return "non-manifold edge";
			case IssueKind::InconsistentWinding:
				return "inconsistent winding";
			case IssueKind::NonPositiveVolumeComponent:
				return "non-positive-volume component";
		}
		return "unknown issue";
	}

	bool issueLess(
		const gen_model::spaceship::topology::Issue& left,
		const gen_model::spaceship::topology::Issue& right
	) {
		if (left.kind != right.kind)
			return static_cast<int>(left.kind) < static_cast<int>(right.kind);
		if (left.triangleIndex != right.triangleIndex)
			return left.triangleIndex < right.triangleIndex;
		if (left.edgeStart.x != right.edgeStart.x)
			return left.edgeStart.x < right.edgeStart.x;
		if (left.edgeStart.y != right.edgeStart.y)
			return left.edgeStart.y < right.edgeStart.y;
		if (left.edgeStart.z != right.edgeStart.z)
			return left.edgeStart.z < right.edgeStart.z;
		if (left.edgeEnd.x != right.edgeEnd.x)
			return left.edgeEnd.x < right.edgeEnd.x;
		if (left.edgeEnd.y != right.edgeEnd.y)
			return left.edgeEnd.y < right.edgeEnd.y;
		return left.edgeEnd.z < right.edgeEnd.z;
	}
}

bool gen_model::spaceship::topology::Report::closedAndOriented() const noexcept {
	return invalidTriangles == 0
		&& degenerateTriangles == 0
		&& boundaryEdges == 0
		&& nonManifoldEdges == 0
		&& inconsistentWindingEdges == 0
		&& nonPositiveVolumeComponents == 0;
}

gen_model::spaceship::topology::Report
gen_model::spaceship::topology::auditClosedOrientedMesh(
	const gen_model::gen_types::MeshData& mesh,
	float weldTolerance
) {
	if (!std::isfinite(weldTolerance) || weldTolerance <= 0.0f)
		throw std::invalid_argument("Spaceship topology weld tolerance must be positive and finite");

	Report report;
	std::unordered_map<EdgeKey, EdgeUse, EdgeKeyHash> edges;
	edges.reserve(mesh.triangles.size() * 2u);

	for (std::size_t triangleIndex = 0; triangleIndex < mesh.triangles.size(); ++triangleIndex) {
		const auto& triangle = mesh.triangles[triangleIndex];
		const bool validIndices = indicesInside(triangle.positionIndices, mesh.positions.size())
			&& indicesInside(triangle.texcoordIndices, mesh.texcoords.size())
			&& indicesInside(triangle.normalIndices, mesh.normals.size());
		if (!validIndices) {
			++report.invalidTriangles;
			report.issues.push_back({IssueKind::InvalidTriangle, triangleIndex});
			continue;
		}

		const std::array<gen_types::Point3, 3> points{
			mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])],
			mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])],
			mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])]
		};
		if (!finite(points[0]) || !finite(points[1]) || !finite(points[2])) {
			++report.invalidTriangles;
			report.issues.push_back({IssueKind::InvalidTriangle, triangleIndex});
			continue;
		}

		const std::array<PositionKey, 3> keys{
			positionKey(points[0], weldTolerance),
			positionKey(points[1], weldTolerance),
			positionKey(points[2], weldTolerance)
		};
		const gen_types::Point3 normal = gen_types::cross(
			points[1] - points[0],
			points[2] - points[0]
		);
		const bool collapsed = keys[0] == keys[1] || keys[1] == keys[2] || keys[2] == keys[0];
		if (collapsed || gen_types::dot(normal, normal) <= 0.000000000001f) {
			++report.degenerateTriangles;
			report.issues.push_back({
				IssueKind::DegenerateTriangle,
				triangleIndex,
				points[0],
				points[1],
				0
			});
			continue;
		}

		for (std::size_t corner = 0; corner < 3; ++corner) {
			const std::size_t next = (corner + 1u) % 3u;
			const bool forward = !keyLess(keys[next], keys[corner]);
			const EdgeKey edgeKey{
				forward ? keys[corner] : keys[next],
				forward ? keys[next] : keys[corner]
			};
			EdgeUse& use = edges[edgeKey];
			if (use.uses == 0) {
				use.start = forward ? points[corner] : points[next];
				use.end = forward ? points[next] : points[corner];
				use.firstTriangle = triangleIndex;
			}
			if (use.uses < use.triangles.size())
				use.triangles[use.uses] = triangleIndex;
			++use.uses;
			use.orientationBalance += forward ? 1 : -1;
		}
	}

	for (const auto& [edge, use] : edges) {
		(void)edge;
		if (use.uses == 1) {
			++report.boundaryEdges;
			report.issues.push_back({
				IssueKind::BoundaryEdge,
				use.firstTriangle,
				use.start,
				use.end,
				use.uses
			});
			continue;
		}
		if (use.uses > 2) {
			++report.nonManifoldEdges;
			report.issues.push_back({
				IssueKind::NonManifoldEdge,
				use.firstTriangle,
				use.start,
				use.end,
				use.uses
			});
			continue;
		}
		if (use.orientationBalance != 0) {
			++report.inconsistentWindingEdges;
			report.issues.push_back({
				IssueKind::InconsistentWinding,
				use.firstTriangle,
				use.start,
				use.end,
				use.uses
			});
		}
	}

	if (report.invalidTriangles == 0
		&& report.degenerateTriangles == 0
		&& report.boundaryEdges == 0
		&& report.nonManifoldEdges == 0
		&& report.inconsistentWindingEdges == 0) {
		std::vector<std::vector<std::size_t>> adjacency(mesh.triangles.size());
		for (const auto& [edge, use] : edges) {
			(void)edge;
			if (use.uses != 2)
				continue;
			adjacency[use.triangles[0]].push_back(use.triangles[1]);
			adjacency[use.triangles[1]].push_back(use.triangles[0]);
		}
		std::vector<bool> visited(mesh.triangles.size(), false);
		std::vector<std::size_t> pending;
		for (std::size_t start = 0; start < mesh.triangles.size(); ++start) {
			if (visited[start])
				continue;
			const std::size_t componentStart = start;
			double signedVolumeTimesSix = 0.0;
			pending.push_back(start);
			visited[start] = true;
			while (!pending.empty()) {
				const std::size_t triangleIndex = pending.back();
				pending.pop_back();
				const auto& triangle = mesh.triangles[triangleIndex];
				const auto& a = mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])];
				const auto& b = mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])];
				const auto& c = mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])];
				signedVolumeTimesSix += static_cast<double>(gen_types::dot(
					a,
					gen_types::cross(b, c)
				));
				for (const std::size_t neighbor : adjacency[triangleIndex]) {
					if (visited[neighbor])
						continue;
					visited[neighbor] = true;
					pending.push_back(neighbor);
				}
			}
			if (signedVolumeTimesSix > 0.000000000001)
				continue;
			++report.nonPositiveVolumeComponents;
			report.issues.push_back({
				IssueKind::NonPositiveVolumeComponent,
				componentStart
			});
		}
	}

	std::sort(report.issues.begin(), report.issues.end(), issueLess);
	return report;
}

void gen_model::spaceship::topology::requireClosedOrientedMesh(
	const gen_model::gen_types::MeshData& mesh,
	float weldTolerance
) {
	const Report report = auditClosedOrientedMesh(mesh, weldTolerance);
	if (report.closedAndOriented())
		return;

	std::ostringstream message;
	message << "Spaceship mesh topology validation failed: invalid="
		<< report.invalidTriangles
		<< ", degenerate=" << report.degenerateTriangles
		<< ", boundary=" << report.boundaryEdges
		<< ", nonManifold=" << report.nonManifoldEdges
		<< ", inconsistentWinding=" << report.inconsistentWindingEdges
		<< ", nonPositiveVolume=" << report.nonPositiveVolumeComponents;
	if (!report.issues.empty()) {
		const Issue& issue = report.issues.front();
		message << "; first=" << issueName(issue.kind)
			<< " triangle=" << issue.triangleIndex;
		if (issue.kind != IssueKind::InvalidTriangle
			&& issue.kind != IssueKind::NonPositiveVolumeComponent) {
			message << std::fixed << std::setprecision(6)
				<< " edge=(" << issue.edgeStart.x << ',' << issue.edgeStart.y << ',' << issue.edgeStart.z
				<< ")->(" << issue.edgeEnd.x << ',' << issue.edgeEnd.y << ',' << issue.edgeEnd.z << ')'
				<< " uses=" << issue.edgeUseCount;
		}
	}
	throw std::invalid_argument(message.str());
}

namespace {
	struct Point2D {
		float x = 0.0f;
		float y = 0.0f;
	};

	float polygonArea2D(const std::vector<Point2D>& pts) {
		if (pts.size() < 3)
			return 0.0f;
		float area = 0.0f;
		for (std::size_t i = 0; i < pts.size(); ++i) {
			const std::size_t j = (i + 1u) % pts.size();
			area += pts[i].x * pts[j].y - pts[j].x * pts[i].y;
		}
		return std::abs(area) * 0.5f;
	}

	std::vector<Point2D> clipPolygonHalfPlane(
		const std::vector<Point2D>& pts,
		Point2D p1,
		Point2D p2
	) {
		std::vector<Point2D> output;
		if (pts.empty())
			return output;
		const float dx = p2.x - p1.x;
		const float dy = p2.y - p1.y;
		auto inside = [&](Point2D p) {
			return dx * (p.y - p1.y) - dy * (p.x - p1.x) >= -1e-7f;
		};
		auto intersection = [&](Point2D cp1, Point2D cp2) {
			const float dcx = cp2.x - cp1.x;
			const float dcy = cp2.y - cp1.y;
			const float denom = dx * dcy - dy * dcx;
			if (std::abs(denom) < 1e-12f)
				return cp1;
			const float t = (dx * (cp1.y - p1.y) - dy * (cp1.x - p1.x)) / (-denom);
			return Point2D{cp1.x + t * dcx, cp1.y + t * dcy};
		};

		Point2D s = pts.back();
		for (const auto& p : pts) {
			if (inside(p)) {
				if (!inside(s))
					output.push_back(intersection(s, p));
				output.push_back(p);
			} else if (inside(s)) {
				output.push_back(intersection(s, p));
			}
			s = p;
		}
		return output;
	}

	float triangleIntersectionArea2D(
		std::array<Point2D, 3> t1,
		std::array<Point2D, 3> t2
	) {
		const float dx = t2[1].x - t2[0].x;
		const float dy = t2[1].y - t2[0].y;
		if (dx * (t2[2].y - t2[0].y) - dy * (t2[2].x - t2[0].x) < 0.0f)
			std::swap(t2[1], t2[2]);
		std::vector<Point2D> poly(t1.begin(), t1.end());
		for (std::size_t i = 0; i < 3; ++i) {
			poly = clipPolygonHalfPlane(poly, t2[i], t2[(i + 1u) % 3u]);
			if (poly.empty())
				return 0.0f;
		}
		return polygonArea2D(poly);
	}

	struct TrianglePlaneInfo {
		std::size_t originalIndex = 0;
		std::array<int, 3> positionIndices{};
		gen_model::gen_types::Point3 normal{};
		float planeDistance = 0.0f;
		gen_model::gen_types::Point3 minPos{};
		gen_model::gen_types::Point3 maxPos{};
		std::array<gen_model::gen_types::Point3, 3> points{};
	};
}

gen_model::spaceship::topology::CoplanarReport
gen_model::spaceship::topology::auditCoplanarZFighting(
	const gen_model::gen_types::MeshData& mesh,
	float normalCosThreshold,
	float planeDistanceThreshold,
	float minAreaOverlap
) {
	CoplanarReport report;
	report.totalTriangles = mesh.triangles.size();

	std::vector<TrianglePlaneInfo> planes;
	planes.reserve(mesh.triangles.size());

	for (std::size_t triangleIndex = 0; triangleIndex < mesh.triangles.size(); ++triangleIndex) {
		const auto& triangle = mesh.triangles[triangleIndex];
		const bool validIndices = indicesInside(triangle.positionIndices, mesh.positions.size());
		if (!validIndices)
			continue;

		const std::array<gen_types::Point3, 3> pts{
			mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])],
			mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])],
			mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])]
		};
		if (!finite(pts[0]) || !finite(pts[1]) || !finite(pts[2]))
			continue;

		const gen_types::Point3 rawNormal = gen_types::cross(pts[1] - pts[0], pts[2] - pts[0]);
		const float normalLengthSq = gen_types::dot(rawNormal, rawNormal);
		if (normalLengthSq <= 1e-12f)
			continue;

		const gen_types::Point3 normal = gen_types::normalize(rawNormal);
		const float planeDistance = gen_types::dot(normal, pts[0]);

		gen_types::Point3 minPos{
			std::min({pts[0].x, pts[1].x, pts[2].x}),
			std::min({pts[0].y, pts[1].y, pts[2].y}),
			std::min({pts[0].z, pts[1].z, pts[2].z})
		};
		gen_types::Point3 maxPos{
			std::max({pts[0].x, pts[1].x, pts[2].x}),
			std::max({pts[0].y, pts[1].y, pts[2].y}),
			std::max({pts[0].z, pts[1].z, pts[2].z})
		};

		planes.push_back({
			triangleIndex,
			triangle.positionIndices,
			normal,
			planeDistance,
			minPos,
			maxPos,
			pts
		});
	}

	std::sort(
		planes.begin(),
		planes.end(),
		[](const TrianglePlaneInfo& left, const TrianglePlaneInfo& right) {
			return left.planeDistance < right.planeDistance;
		}
	);

	for (std::size_t i = 0; i < planes.size(); ++i) {
		const auto& p1 = planes[i];
		for (std::size_t j = i + 1; j < planes.size(); ++j) {
			const auto& p2 = planes[j];
			if (p2.planeDistance - p1.planeDistance > planeDistanceThreshold)
				break;

			const float cosAngle = gen_types::dot(p1.normal, p2.normal);
			if (cosAngle < normalCosThreshold)
				continue;

			if (p1.minPos.x > p2.maxPos.x + planeDistanceThreshold
				|| p1.maxPos.x < p2.minPos.x - planeDistanceThreshold
				|| p1.minPos.y > p2.maxPos.y + planeDistanceThreshold
				|| p1.maxPos.y < p2.minPos.y - planeDistanceThreshold
				|| p1.minPos.z > p2.maxPos.z + planeDistanceThreshold
				|| p1.maxPos.z < p2.minPos.z - planeDistanceThreshold)
				continue;

			// Project to 2D by dropping dominant axis
			const float absX = std::abs(p1.normal.x);
			const float absY = std::abs(p1.normal.y);
			const float absZ = std::abs(p1.normal.z);
			int dropAxis = 2;
			if (absX >= absY && absX >= absZ)
				dropAxis = 0;
			else if (absY >= absX && absY >= absZ)
				dropAxis = 1;

			auto to2D = [dropAxis](gen_types::Point3 p) {
				if (dropAxis == 0)
					return Point2D{p.y, p.z};
				if (dropAxis == 1)
					return Point2D{p.x, p.z};
				return Point2D{p.x, p.y};
			};

			std::array<Point2D, 3> t1{to2D(p1.points[0]), to2D(p1.points[1]), to2D(p1.points[2])};
			std::array<Point2D, 3> t2{to2D(p2.points[0]), to2D(p2.points[1]), to2D(p2.points[2])};

			std::array<int, 3> sorted1 = p1.positionIndices;
			std::array<int, 3> sorted2 = p2.positionIndices;
			std::sort(sorted1.begin(), sorted1.end());
			std::sort(sorted2.begin(), sorted2.end());
			if (sorted1 == sorted2) {
				++report.coplanarPairs;
				report.issues.push_back({
					p1.originalIndex,
					p2.originalIndex,
					cosAngle,
					p2.planeDistance - p1.planeDistance,
					polygonArea2D({t1[0], t1[1], t1[2]}),
					p1.points[0]
				});
				continue;
			}

			const float overlapArea = triangleIntersectionArea2D(t1, t2);
			if (overlapArea >= minAreaOverlap) {
				++report.coplanarPairs;
				report.issues.push_back({
					p1.originalIndex,
					p2.originalIndex,
					cosAngle,
					p2.planeDistance - p1.planeDistance,
					overlapArea,
					p1.points[0]
				});
			}
		}
	}

	return report;
}

void gen_model::spaceship::topology::requireNoCoplanarZFighting(
	const gen_model::gen_types::MeshData& mesh,
	float normalCosThreshold,
	float planeDistanceThreshold,
	float minAreaOverlap
) {
	const CoplanarReport report = auditCoplanarZFighting(
		mesh,
		normalCosThreshold,
		planeDistanceThreshold,
		minAreaOverlap
	);
	if (report.clean())
		return;

	std::ostringstream message;
	message << "Coplanar Z-fighting hazard detected: "
		<< report.coplanarPairs << " overlapping pair(s) found in mesh.";
	if (!report.issues.empty()) {
		const auto& first = report.issues.front();
		message << std::fixed << std::setprecision(5)
			<< " First issue: Tri " << first.triangleA << " and Tri " << first.triangleB
			<< " on plane dist delta=" << first.planeDistanceDelta
			<< ", normal dot=" << first.normalDot
			<< ", overlap area=" << first.overlapArea
			<< " at approx (" << first.intersectionSample.x << ", "
			<< first.intersectionSample.y << ", " << first.intersectionSample.z << ")";
	}
	throw std::invalid_argument(message.str());
}

namespace {
	bool triTriIntersect3D(
		const gen_model::gen_types::Point3& v0,
		const gen_model::gen_types::Point3& v1,
		const gen_model::gen_types::Point3& v2,
		const gen_model::gen_types::Point3& u0,
		const gen_model::gen_types::Point3& u1,
		const gen_model::gen_types::Point3& u2,
		float eps
	) {
		using namespace gen_model::gen_types;
		const Point3 n2 = cross(u1 - u0, u2 - u0);
		const float lenN2 = std::sqrt(dot(n2, n2));
		if (lenN2 < 1e-9f)
			return false;
		const Point3 n2Norm = n2 * (1.0f / lenN2);
		const float d2 = -dot(n2Norm, u0);

		const float du0 = dot(n2Norm, v0) + d2;
		const float du1 = dot(n2Norm, v1) + d2;
		const float du2 = dot(n2Norm, v2) + d2;

		if ((du0 >= -eps && du1 >= -eps && du2 >= -eps) || (du0 <= eps && du1 <= eps && du2 <= eps))
			return false;
		if (std::abs(du0) <= eps && std::abs(du1) <= eps && std::abs(du2) <= eps)
			return false; // Coplanar handled separately

		const Point3 n1 = cross(v1 - v0, v2 - v0);
		const float lenN1 = std::sqrt(dot(n1, n1));
		if (lenN1 < 1e-9f)
			return false;
		const Point3 n1Norm = n1 * (1.0f / lenN1);
		const float d1 = -dot(n1Norm, v0);

		const float dv0 = dot(n1Norm, u0) + d1;
		const float dv1 = dot(n1Norm, u1) + d1;
		const float dv2 = dot(n1Norm, u2) + d1;

		if ((dv0 >= -eps && dv1 >= -eps && dv2 >= -eps) || (dv0 <= eps && dv1 <= eps && dv2 <= eps))
			return false;
		if (std::abs(dv0) <= eps && std::abs(dv1) <= eps && std::abs(dv2) <= eps)
			return false;

		const Point3 d = cross(n1Norm, n2Norm);
		int maxD = 0;
		float maxVal = std::abs(d.x);
		if (std::abs(d.y) > maxVal) { maxVal = std::abs(d.y); maxD = 1; }
		if (std::abs(d.z) > maxVal) { maxVal = std::abs(d.z); maxD = 2; }
		if (maxVal < 1e-9f)
			return false;

		auto getCoord = [maxD](const Point3& p) {
			return (maxD == 0) ? p.x : (maxD == 1 ? p.y : p.z);
		};

		auto getInterval = [&](
			const Point3& vv0, const Point3& vv1, const Point3& vv2,
			float d0, float d1, float d2,
			float& t0, float& t1
		) {
			float p0, p1, p2, dp0, dp1, dp2;
			if (d0 * d1 > 0.0f) {
				p0 = getCoord(vv0); p1 = getCoord(vv1); p2 = getCoord(vv2);
				dp0 = d0; dp1 = d1; dp2 = d2;
			} else if (d0 * d2 > 0.0f) {
				p0 = getCoord(vv0); p1 = getCoord(vv2); p2 = getCoord(vv1);
				dp0 = d0; dp1 = d2; dp2 = d1;
			} else {
				p0 = getCoord(vv1); p1 = getCoord(vv2); p2 = getCoord(vv0);
				dp0 = d1; dp1 = d2; dp2 = d0;
			}
			const float den0 = dp2 - dp0;
			const float den1 = dp2 - dp1;
			t0 = (std::abs(den0) > 1e-9f) ? p2 + (p0 - p2) * (dp2 / den0) : p2;
			t1 = (std::abs(den1) > 1e-9f) ? p2 + (p1 - p2) * (dp2 / den1) : p2;
			if (t0 > t1)
				std::swap(t0, t1);
		};

		float t0Min = 0.0f, t0Max = 0.0f;
		float t1Min = 0.0f, t1Max = 0.0f;
		getInterval(v0, v1, v2, du0, du1, du2, t0Min, t0Max);
		getInterval(u0, u1, u2, dv0, dv1, dv2, t1Min, t1Max);

		const float overlap = std::min(t0Max, t1Max) - std::max(t0Min, t1Min);
		return overlap > eps;
	}
}

gen_model::spaceship::topology::IntersectionReport
gen_model::spaceship::topology::auditMeshIntersections(
	const gen_model::gen_types::MeshData& mesh,
	float tolerance
) {
	IntersectionReport report;
	report.totalTriangles = mesh.triangles.size();

	// Weld positions by tolerance
	std::unordered_map<PositionKey, int, PositionKeyHash> keyToIndex;
	keyToIndex.reserve(mesh.positions.size());
	std::vector<int> remap(mesh.positions.size(), -1);

	for (std::size_t i = 0; i < mesh.positions.size(); ++i) {
		const auto key = positionKey(mesh.positions[i], 0.001f);
		auto it = keyToIndex.find(key);
		if (it == keyToIndex.end()) {
			const int newIdx = static_cast<int>(keyToIndex.size());
			keyToIndex.emplace(key, newIdx);
			remap[i] = newIdx;
		} else {
			remap[i] = it->second;
		}
	}

	struct TriInfo {
		std::size_t origIdx;
		std::array<int, 3> weldedIndices;
		std::array<gen_types::Point3, 3> pts;
		gen_types::Point3 minPos;
		gen_types::Point3 maxPos;
	};

	std::vector<TriInfo> tris;
	tris.reserve(mesh.triangles.size());

	for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
		const auto& tri = mesh.triangles[i];
		if (!indicesInside(tri.positionIndices, mesh.positions.size()))
			continue;

		const std::array<gen_types::Point3, 3> pts{
			mesh.positions[static_cast<std::size_t>(tri.positionIndices[0])],
			mesh.positions[static_cast<std::size_t>(tri.positionIndices[1])],
			mesh.positions[static_cast<std::size_t>(tri.positionIndices[2])]
		};
		if (!finite(pts[0]) || !finite(pts[1]) || !finite(pts[2]))
			continue;

		const std::array<int, 3> welded{
			remap[static_cast<std::size_t>(tri.positionIndices[0])],
			remap[static_cast<std::size_t>(tri.positionIndices[1])],
			remap[static_cast<std::size_t>(tri.positionIndices[2])]
		};

		// Skip degenerate welded triangles
		if (welded[0] == welded[1] || welded[1] == welded[2] || welded[0] == welded[2])
			continue;

		gen_types::Point3 minP{
			std::min({pts[0].x, pts[1].x, pts[2].x}),
			std::min({pts[0].y, pts[1].y, pts[2].y}),
			std::min({pts[0].z, pts[1].z, pts[2].z})
		};
		gen_types::Point3 maxP{
			std::max({pts[0].x, pts[1].x, pts[2].x}),
			std::max({pts[0].y, pts[1].y, pts[2].y}),
			std::max({pts[0].z, pts[1].z, pts[2].z})
		};

		tris.push_back({i, welded, pts, minP, maxP});
	}

	for (std::size_t i = 0; i < tris.size(); ++i) {
		const auto& tA = tris[i];
		for (std::size_t j = i + 1; j < tris.size(); ++j) {
			const auto& tB = tris[j];

			// If they share any welded vertex, they are adjacent/connected -> skip
			if (tA.weldedIndices[0] == tB.weldedIndices[0] || tA.weldedIndices[0] == tB.weldedIndices[1] || tA.weldedIndices[0] == tB.weldedIndices[2]
				|| tA.weldedIndices[1] == tB.weldedIndices[0] || tA.weldedIndices[1] == tB.weldedIndices[1] || tA.weldedIndices[1] == tB.weldedIndices[2]
				|| tA.weldedIndices[2] == tB.weldedIndices[0] || tA.weldedIndices[2] == tB.weldedIndices[1] || tA.weldedIndices[2] == tB.weldedIndices[2]) {
				continue;
			}

			// Bounding box rejection
			if (tA.minPos.x > tB.maxPos.x + tolerance || tA.maxPos.x < tB.minPos.x - tolerance
				|| tA.minPos.y > tB.maxPos.y + tolerance || tA.maxPos.y < tB.minPos.y - tolerance
				|| tA.minPos.z > tB.maxPos.z + tolerance || tA.maxPos.z < tB.minPos.z - tolerance) {
				continue;
			}

			if (triTriIntersect3D(tA.pts[0], tA.pts[1], tA.pts[2], tB.pts[0], tB.pts[1], tB.pts[2], tolerance)) {
				++report.intersectingPairs;
				report.issues.push_back({tA.origIdx, tB.origIdx, tA.pts[0]});
			}
		}
	}

	return report;
}

void gen_model::spaceship::topology::requireNoMeshIntersections(
	const gen_model::gen_types::MeshData& mesh,
	float tolerance
) {
	const IntersectionReport report = auditMeshIntersections(mesh, tolerance);
	if (report.clean())
		return;

	std::ostringstream message;
	message << "Mesh self-intersection hazard detected: "
		<< report.intersectingPairs << " intersecting pair(s) found in mesh.";
	if (!report.issues.empty()) {
		const auto& first = report.issues.front();
		message << " First issue: Tri " << first.triangleA << " and Tri " << first.triangleB;
	}
	throw std::invalid_argument(message.str());
}

namespace {
	float segmentSegmentDistSq(
		const Point3& p1, const Point3& p2,
		const Point3& p3, const Point3& p4
	) {
		const Point3 u = p2 - p1;
		const Point3 v = p4 - p3;
		const Point3 w = p1 - p3;
		const float a = dot(u, u);
		const float b = dot(u, v);
		const float c = dot(v, v);
		const float d = dot(u, w);
		const float e = dot(v, w);
		const float D = a * c - b * b;
		float sc, sN, sD = D;
		float tc, tN, tD = D;
		if (D < 1e-8f) {
			sN = 0.0f; sD = 1.0f; tN = e; tD = c;
		} else {
			sN = (b * e - c * d);
			tN = (a * e - b * d);
			if (sN < 0.0f) {
				sN = 0.0f; tN = e; tD = c;
			} else if (sN > sD) {
				sN = sD; tN = e + b; tD = c;
			}
		}
		if (tN < 0.0f) {
			tN = 0.0f;
			if (-d < 0.0f) sN = 0.0f;
			else if (-d > a) sN = sD;
			else { sN = -d; sD = a; }
		} else if (tN > tD) {
			tN = tD;
			if ((-d + b) < 0.0f) sN = 0.0f;
			else if ((-d + b) > a) sN = sD;
			else { sN = (-d + b); sD = a; }
		}
		sc = (std::abs(sN) < 1e-8f ? 0.0f : sN / sD);
		tc = (std::abs(tN) < 1e-8f ? 0.0f : tN / tD);
		const Point3 dP = w + (u * sc) - (v * tc);
		return dot(dP, dP);
	}

	float pointTriangleDistSq(
		const Point3& p,
		const Point3& a, const Point3& b, const Point3& c
	) {
		const Point3 ab = b - a;
		const Point3 ac = c - a;
		const Point3 ap = p - a;
		const float d1 = dot(ab, ap);
		const float d2 = dot(ac, ap);
		if (d1 <= 0.0f && d2 <= 0.0f) return dot(p - a, p - a);
		const Point3 bp = p - b;
		const float d3 = dot(ab, bp);
		const float d4 = dot(ac, bp);
		if (d3 >= 0.0f && d4 <= d3) return dot(p - b, p - b);
		const float vc = d1 * d4 - d3 * d2;
		if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
			const float v = d1 / (d1 - d3);
			const Point3 proj = a + ab * v;
			return dot(p - proj, p - proj);
		}
		const Point3 cp = p - c;
		const float d5 = dot(ab, cp);
		const float d6 = dot(ac, cp);
		if (d6 >= 0.0f && d5 <= d6) return dot(p - c, p - c);
		const float vb = d5 * d2 - d1 * d6;
		if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
			const float w = d2 / (d2 - d6);
			const Point3 proj = a + ac * w;
			return dot(p - proj, p - proj);
		}
		const float va = d3 * d6 - d5 * d4;
		if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
			const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
			const Point3 proj = b + (c - b) * w;
			return dot(p - proj, p - proj);
		}
		const float denom = 1.0f / (va + vb + vc);
		const float v = vb * denom;
		const float w = vc * denom;
		const Point3 proj = a + ab * v + ac * w;
		return dot(p - proj, p - proj);
	}

	float triTriDistanceSq(
		const std::array<Point3, 3>& t1,
		const std::array<Point3, 3>& t2
	) {
		float d2 = std::min({
			pointTriangleDistSq(t1[0], t2[0], t2[1], t2[2]),
			pointTriangleDistSq(t1[1], t2[0], t2[1], t2[2]),
			pointTriangleDistSq(t1[2], t2[0], t2[1], t2[2]),
			pointTriangleDistSq(t2[0], t1[0], t1[1], t1[2]),
			pointTriangleDistSq(t2[1], t1[0], t1[1], t1[2]),
			pointTriangleDistSq(t2[2], t1[0], t1[1], t1[2])
		});
		for (int i = 0; i < 3; ++i) {
			const auto& e1_a = t1[i];
			const auto& e1_b = t1[(i + 1) % 3];
			for (int j = 0; j < 3; ++j) {
				const auto& e2_a = t2[j];
				const auto& e2_b = t2[(j + 1) % 3];
				d2 = std::min(d2, segmentSegmentDistSq(e1_a, e1_b, e2_a, e2_b));
			}
		}
		return d2;
	}
}

gen_model::spaceship::topology::ClearanceReport
gen_model::spaceship::topology::auditSurfaceClearance(
	const gen_model::gen_types::MeshData& mesh,
	float minClearance
) {
	ClearanceReport report;
	report.totalTriangles = mesh.triangles.size();
	if (mesh.triangles.empty()) return report;

	std::vector<PositionKey> vertKeys(mesh.positions.size());
	for (std::size_t i = 0; i < mesh.positions.size(); ++i) {
		vertKeys[i] = positionKey(mesh.positions[i], 0.0001f);
	}

	std::vector<int> parent(mesh.triangles.size());
	std::iota(parent.begin(), parent.end(), 0);
	std::function<int(int)> findRoot = [&](int i) -> int {
		if (parent[i] == i) return i;
		return parent[i] = findRoot(parent[i]);
	};
	auto unionComps = [&](int i, int j) {
		const int pi = findRoot(i);
		const int pj = findRoot(j);
		if (pi != pj) parent[pi] = pj;
	};

	std::unordered_map<PositionKey, std::vector<int>, PositionKeyHash> posToTris;
	for (int tIdx = 0; tIdx < static_cast<int>(mesh.triangles.size()); ++tIdx) {
		for (int corner = 0; corner < 3; ++corner) {
			const int vIdx = mesh.triangles[tIdx].positionIndices[corner];
			posToTris[vertKeys[vIdx]].push_back(tIdx);
		}
	}
	for (const auto& [key, tList] : posToTris) {
		for (std::size_t k = 1; k < tList.size(); ++k) {
			unionComps(tList[0], tList[k]);
		}
	}

	std::vector<int> triComp(mesh.triangles.size());
	std::unordered_map<int, int> compMap;
	for (int tIdx = 0; tIdx < static_cast<int>(mesh.triangles.size()); ++tIdx) {
		const int root = findRoot(tIdx);
		if (!compMap.contains(root)) compMap[root] = static_cast<int>(compMap.size());
		triComp[tIdx] = compMap[root];
	}
	report.totalComponents = compMap.size();

	std::vector<std::array<Point3, 3>> triPts(mesh.triangles.size());
	std::vector<Point3> aabbMin(mesh.triangles.size());
	std::vector<Point3> aabbMax(mesh.triangles.size());
	for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
		const auto& tri = mesh.triangles[i];
		triPts[i] = {
			mesh.positions[tri.positionIndices[0]],
			mesh.positions[tri.positionIndices[1]],
			mesh.positions[tri.positionIndices[2]]
		};
		aabbMin[i] = {
			std::min({triPts[i][0].x, triPts[i][1].x, triPts[i][2].x}),
			std::min({triPts[i][0].y, triPts[i][1].y, triPts[i][2].y}),
			std::min({triPts[i][0].z, triPts[i][1].z, triPts[i][2].z})
		};
		aabbMax[i] = {
			std::max({triPts[i][0].x, triPts[i][1].x, triPts[i][2].x}),
			std::max({triPts[i][0].y, triPts[i][1].y, triPts[i][2].y}),
			std::max({triPts[i][0].z, triPts[i][1].z, triPts[i][2].z})
		};
	}

	const float thresholdSq = (minClearance - 1e-4f) * (minClearance - 1e-4f);
	for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
		for (std::size_t j = i + 1; j < mesh.triangles.size(); ++j) {
			if (triComp[i] == triComp[j]) continue;
			if (aabbMin[i].x - minClearance > aabbMax[j].x || aabbMax[i].x + minClearance < aabbMin[j].x ||
			    aabbMin[i].y - minClearance > aabbMax[j].y || aabbMax[i].y + minClearance < aabbMin[j].y ||
			    aabbMin[i].z - minClearance > aabbMax[j].z || aabbMax[i].z + minClearance < aabbMin[j].z)
				continue;
			const float d2 = triTriDistanceSq(triPts[i], triPts[j]);
			const float dist = std::sqrt(d2);
			if (dist < report.minimumDistance) report.minimumDistance = dist;
			if (d2 < thresholdSq) {
				report.closePairs++;
				if (report.issues.size() < 20) {
					report.issues.push_back({i, j, static_cast<std::size_t>(triComp[i]), static_cast<std::size_t>(triComp[j]), dist});
				}
			}
		}
	}
	return report;
}

