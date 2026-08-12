#include "spaceship_topology.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {
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
