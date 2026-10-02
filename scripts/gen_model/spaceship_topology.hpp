#ifndef GEN_MODEL_SPACESHIP_TOPOLOGY_HPP
#define GEN_MODEL_SPACESHIP_TOPOLOGY_HPP

#include <cstddef>
#include <vector>

#include "gen_types.hpp"

namespace gen_model::spaceship::topology {
	enum class IssueKind {
		InvalidTriangle,
		DegenerateTriangle,
		BoundaryEdge,
		NonManifoldEdge,
		InconsistentWinding,
		NonPositiveVolumeComponent
	};

	struct Issue {
		IssueKind kind = IssueKind::InvalidTriangle;
		std::size_t triangleIndex = 0;
		gen_types::Point3 edgeStart{};
		gen_types::Point3 edgeEnd{};
		std::size_t edgeUseCount = 0;
	};

	struct Report {
		std::size_t invalidTriangles = 0;
		std::size_t degenerateTriangles = 0;
		std::size_t boundaryEdges = 0;
		std::size_t nonManifoldEdges = 0;
		std::size_t inconsistentWindingEdges = 0;
		std::size_t nonPositiveVolumeComponents = 0;
		std::vector<Issue> issues;

		bool closedAndOriented() const noexcept;
	};

	Report auditClosedOrientedMesh(
		const gen_types::MeshData& mesh,
		float weldTolerance = 0.00001f
	);

	void requireClosedOrientedMesh(
		const gen_types::MeshData& mesh,
		float weldTolerance = 0.00001f
	);

	struct CoplanarIssue {
		std::size_t triangleA = 0;
		std::size_t triangleB = 0;
		float normalDot = 0.0f;
		float planeDistanceDelta = 0.0f;
		float overlapArea = 0.0f;
		gen_types::Point3 intersectionSample{};
	};

	struct CoplanarReport {
		std::size_t totalTriangles = 0;
		std::size_t coplanarPairs = 0;
		std::vector<CoplanarIssue> issues;

		[[nodiscard]] bool clean() const noexcept {
			return coplanarPairs == 0;
		}
	};

	struct IntersectionIssue {
		std::size_t triangleA = 0;
		std::size_t triangleB = 0;
		gen_types::Point3 samplePoint{};
	};

	struct IntersectionReport {
		std::size_t totalTriangles = 0;
		std::size_t intersectingPairs = 0;
		std::vector<IntersectionIssue> issues;

		[[nodiscard]] bool clean() const noexcept {
			return intersectingPairs == 0;
		}
	};

	struct ClearanceIssue {
		std::size_t triangleA = 0;
		std::size_t triangleB = 0;
		std::size_t componentA = 0;
		std::size_t componentB = 0;
		float distance = 0.0f;
	};

	struct ClearanceReport {
		std::size_t totalTriangles = 0;
		std::size_t totalComponents = 0;
		std::size_t closePairs = 0;
		float minimumDistance = 999.0f;
		std::vector<ClearanceIssue> issues;

		[[nodiscard]] bool clean() const noexcept {
			return closePairs == 0;
		}
	};

	CoplanarReport auditCoplanarZFighting(
		const gen_types::MeshData& mesh,
		float normalCosThreshold = 0.999f,
		float planeDistanceThreshold = 0.005f,
		float minAreaOverlap = 0.00001f
	);

	IntersectionReport auditMeshIntersections(
		const gen_types::MeshData& mesh,
		float tolerance = 1e-5f
	);

	ClearanceReport auditSurfaceClearance(
		const gen_types::MeshData& mesh,
		float minClearance = 0.05f
	);

	void requireNoMeshIntersections(
		const gen_types::MeshData& mesh,
		float tolerance = 1e-5f
	);

	void requireNoCoplanarZFighting(
		const gen_types::MeshData& mesh,
		float normalCosThreshold = 0.999f,
		float planeDistanceThreshold = 0.005f,
		float minAreaOverlap = 0.00001f
	);
} // namespace gen_model::spaceship::topology

#endif // GEN_MODEL_SPACESHIP_TOPOLOGY_HPP
