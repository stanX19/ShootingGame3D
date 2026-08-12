#ifndef GEN_MODEL_SPACESHIP_DESIGN_HPP
#define GEN_MODEL_SPACESHIP_DESIGN_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "spaceship_generator.hpp"

namespace gen_model::spaceship::design {
	enum class ModuleKind {
		Cockpit,
		EngineCore,
		FuelTank,
		Reactor,
		Magazine,
		ServiceBay,
		Radiator,
		MountFoundation
	};

	enum class ModuleShape {
		FacetedCapsule,
		ArmoredWedge,
		Ellipsoid,
		CappedCylinder,
		ShieldedSphere,
		ChamferedBox,
		TaperedBox,
		RadialWedge,
		AxialFrustum,
		TaperedBeam
	};

	struct ModuleVolume {
		ModuleKind kind = ModuleKind::ServiceBay;
		ModuleShape shape = ModuleShape::ChamferedBox;
		gen_types::Point3 center{};
		gen_types::Point3 halfExtents{};
		gen_types::Point3 forward{0.0f, 0.0f, 1.0f};
		gen_types::Point3 accessDirection{0.0f, 0.0f, 1.0f};
		gen_types::Point3 heatDirection{0.0f, -1.0f, 0.0f};
		float requiredVolume = 0.0f;
		float actualVolume = 0.0f;
		float chamfer = 0.0f;
		float mass = 0.0f;
		int ownerIndex = -1;
		bool protectedByEnvelope = true;
		bool operator==(const ModuleVolume&) const = default;
	};

	struct StructuralNode {
		gen_types::Point3 position{};
		float load = 0.0f;
		int moduleIndex = -1;
		bool operator==(const StructuralNode&) const = default;
	};

	struct StructuralLink {
		std::size_t startNode = 0;
		std::size_t endNode = 0;
		float thickness = 0.0f;
		bool operator==(const StructuralLink&) const = default;
	};

	struct EnginePod {
		EngineSettings runtime{};
		gen_types::Point3 forward{0.0f, 0.0f, -1.0f};
		float nozzleRadius = 0.0f;
		float thrustCapacity = 0.0f;
		int nozzleCells = 1;
		bool operator==(const EnginePod&) const = default;
	};

	struct EnvelopeStation {
		float z = 0.0f;
		float halfWidth = 0.0f;
		float top = 0.0f;
		float bottom = 0.0f;
		float shoulder = 0.0f;
		bool operator==(const EnvelopeStation&) const = default;
	};

	struct SurfaceSample {
		gen_types::Point3 point{};
		gen_types::Point3 normal{0.0f, 1.0f, 0.0f};
		gen_types::Point3 tangent{1.0f, 0.0f, 0.0f};
		float structuralDepth = 0.0f;
		int region = 0;
		bool operator==(const SurfaceSample&) const = default;
	};

	struct PreliminaryDesign {
		DesignMetrics metrics{};
		std::vector<ModuleVolume> coreModules;
		std::vector<StructuralNode> nodes;
		std::vector<StructuralLink> links;
		std::vector<EnginePod> enginePods;
		std::vector<EnvelopeStation> centralStations;
		std::vector<SurfaceSample> candidateSurfaces;
		bool operator==(const PreliminaryDesign&) const = default;
	};

	struct DesignPlan : PreliminaryDesign {
		std::vector<ModuleVolume> weaponModules;
		std::vector<MountSettings> resolvedMounts;
		bool operator==(const DesignPlan&) const = default;
	};

	struct Audit {
		std::size_t invalidShapeVolumes = 0;
		std::size_t uncontainedModules = 0;
		std::size_t disconnectedNodes = 0;
		std::size_t thermalViolations = 0;
		std::size_t accessViolations = 0;
		std::size_t centroidViolations = 0;
		std::size_t propulsionViolations = 0;
		std::size_t podCapacityViolations = 0;
		std::size_t podFitViolations = 0;
		std::vector<std::string> diagnostics;

		bool valid() const noexcept;
	};

	PreliminaryDesign planCore(const Settings& settings);
	DesignPlan complete(
		const Settings& settings,
		const PreliminaryDesign& preliminary,
		const std::vector<ModuleVolume>& weaponModules,
		const std::vector<MountSettings>& resolvedMounts
	);
	Audit audit(const Settings& settings, const DesignPlan& plan);
	void requireValid(const Settings& settings, const DesignPlan& plan);
	const char* layoutName(PropulsionLayout layout) noexcept;
	bool shapeAllowedFor(ModuleKind kind, ModuleShape shape) noexcept;
} // namespace gen_model::spaceship::design

#endif // GEN_MODEL_SPACESHIP_DESIGN_HPP
