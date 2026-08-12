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
} // namespace gen_model::spaceship::topology

#endif // GEN_MODEL_SPACESHIP_TOPOLOGY_HPP
