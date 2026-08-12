#ifndef GEN_MODEL_SPACESHIP_MESH_HPP
#define GEN_MODEL_SPACESHIP_MESH_HPP

#include <array>
#include <optional>
#include <vector>

#include "gen_types.hpp"

namespace gen_model::spaceship::detail {
	enum class Surface {
		Armor,
		Structure,
		Canopy,
		Engine,
		Socket
	};

	struct TriangleTag {
		Surface surface = Surface::Armor;
		int mountOwner = -1;
		bool socket = false;
	};

	struct TaggedMesh {
		gen_types::MeshData mesh;
		std::vector<TriangleTag> tags;
	};

	struct PlanPoint {
		float x;
		float z;
	};

	class MeshBuilder {
	public:
		void addTriangle(
			gen_types::Point3 a,
			gen_types::Point3 b,
			gen_types::Point3 c,
			gen_types::Point3 expectedNormal,
			Surface surface,
			int mountOwner = -1,
			bool socket = false
		);
		void addQuad(
			gen_types::Point3 a,
			gen_types::Point3 b,
			gen_types::Point3 c,
			gen_types::Point3 d,
			gen_types::Point3 expectedNormal,
			Surface surface,
			int mountOwner = -1,
			bool socket = false
		);
		void addLoftSides(
			const std::vector<std::vector<gen_types::Point3>>& rings,
			Surface surface,
			int mountOwner = -1,
			bool socket = false
		);
		void addConvexCap(
			const std::vector<gen_types::Point3>& ring,
			gen_types::Point3 expectedNormal,
			Surface surface,
			int mountOwner = -1,
			bool socket = false
		);
		void addClosedLoft(
			const std::vector<std::vector<gen_types::Point3>>& rings,
			Surface sideSurface,
			Surface startCapSurface,
			Surface endCapSurface,
			int mountOwner = -1,
			bool socket = false
		);
		void addBox(gen_types::Point3 center, gen_types::Point3 size, Surface surface);
		void addTaperedBeam(
			gen_types::Point3 start,
			gen_types::Point3 end,
			float startWidth,
			float endWidth,
			float height,
			Surface surface,
			int mountOwner = -1
		);
		void addPrismY(
			const std::vector<PlanPoint>& plan,
			float bottomY,
			float topY,
			Surface surface
		);
		void addCylinderY(
			gen_types::Point3 center,
			float radius,
			float height,
			int segments,
			Surface surface,
			int mountOwner = -1,
			bool socket = false
		);
		void addCylinderZ(
			gen_types::Point3 center,
			float radius,
			float length,
			int segments,
			Surface surface
		);
		std::optional<float> topStructuralSurfaceY(float x, float z) const;
		TaggedMesh finish();

	private:
		TaggedMesh result;
	};
} // namespace gen_model::spaceship::detail

#endif // GEN_MODEL_SPACESHIP_MESH_HPP
