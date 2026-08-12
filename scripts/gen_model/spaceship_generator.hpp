#ifndef GEN_MODEL_SPACESHIP_GENERATOR_HPP
#define GEN_MODEL_SPACESHIP_GENERATOR_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "gen_types.hpp"

namespace gen_model::spaceship {
	struct Dimensions {
		float width = 8.0f;
		float height = 2.4f;
		float length = 4.8f;
		bool operator==(const Dimensions&) const = default;
	};

	struct HullSettings {
		float width = 2.4f;
		float height = 1.4f;
		float length = 4.6f;
		float crown = 0.18f;
		float keel = 0.12f;
		float noseSharpness = 0.72f;
		float rearTaper = 0.70f;
		bool operator==(const HullSettings&) const = default;
	};

	struct WingSettings {
		float halfSpan = 3.55f;
		float rootFrontZ = 1.10f;
		float rootRearZ = -1.90f;
		float tipFrontZ = -0.15f;
		float tipRearZ = -2.15f;
		float rootX = 0.85f;
		float topY = -0.08f;
		float bottomY = -0.34f;
		float shoulderWidth = 0.55f;
		bool operator==(const WingSettings&) const = default;
	};

	struct CockpitSettings {
		gen_types::Point3 center{0.0f, 0.72f, 0.82f};
		gen_types::Point3 size{1.10f, 0.58f, 1.55f};
		float browDepth = 0.12f;
		bool operator==(const CockpitSettings&) const = default;
	};

	struct EngineSettings {
		gen_types::Point3 center{};
		float radius = 0.34f;
		float length = 1.35f;
		float nozzleDepth = 0.22f;
		bool operator==(const EngineSettings&) const = default;
	};

	struct WearSettings {
		float paintLoss = 0.18f;
		float oxidation = 0.07f;
		float heatStaining = 0.22f;
		int repairPanels = 3;
		bool operator==(const WearSettings&) const = default;
	};

	struct RgbColor {
		std::uint8_t r = 0u;
		std::uint8_t g = 0u;
		std::uint8_t b = 0u;
		bool operator==(const RgbColor&) const = default;
	};

	enum class PaintPattern {
		SpineBand,
		Chevron,
		WingBands,
		Blocked,
		Hazard
	};

	struct MaterialSettings {
		RgbColor armorBase{112u, 126u, 138u};
		RgbColor armorSecondary{54u, 66u, 78u};
		RgbColor accent{202u, 136u, 42u};
		RgbColor structure{52u, 60u, 68u};
		RgbColor canopy{18u, 96u, 124u};
		RgbColor engine{62u, 154u, 204u};
		PaintPattern pattern = PaintPattern::Chevron;
		float secondaryCoverage = 0.30f;
		float accentCoverage = 0.09f;
		float normalStrength = 0.11f;
		float detailScale = 1.0f;
		bool operator==(const MaterialSettings&) const = default;
	};

	struct LayoutSettings {
		std::string archetype = "multirole";
		int crew = 1;
		float primarySpineWidth = 0.55f;
		float reactorRadius = 0.50f;
		float serviceBayLength = 1.20f;
		float radiatorScale = 0.50f;
		float weaponDeckCantDegrees = 0.0f;
		bool operator==(const LayoutSettings&) const = default;
	};

	struct MountAttachmentSettings {
		float preferredTakeoffAngleDegrees = 30.0f;
		float minimumTakeoffAngleDegrees = 25.0f;
		float maximumTakeoffAngleDegrees = 40.0f;
		float directBlisterGapScale = 1.8f;
		float distanceWeight = 0.35f;
		float angleWeight = 1.0f;
		float blisterRadiusScale = 1.35f;
		bool operator==(const MountAttachmentSettings&) const = default;
	};

	enum class PropulsionLayout {
		Auto,
		CentralCluster,
		SpineCluster,
		TwinBoom,
		WingNacelles,
		DistributedAft,
		CapitalSideBlocks
	};

	enum class PlacementMode { Auto, Manual };
	enum class WeaponCoverage { Forward, Broadside, Omnidirectional };
	enum class LayoutSymmetry { Bilateral, Radial, None };
	enum class BatteryStyle { Auto, Integrated, External };

	struct WeaponLayoutSettings {
		PlacementMode placement = PlacementMode::Auto;
		WeaponCoverage coverage = WeaponCoverage::Forward;
		LayoutSymmetry symmetry = LayoutSymmetry::Bilateral;
		BatteryStyle batteryStyle = BatteryStyle::Auto;
		std::size_t turretCount = 0;
		float minimumSeparationScale = 1.25f;
		bool operator==(const WeaponLayoutSettings&) const = default;
	};

	struct DesignSettings {
		PropulsionLayout propulsionLayout = PropulsionLayout::Auto;
		PlacementMode propulsionPlacement = PlacementMode::Auto;
		float targetAcceleration = 1.0f;
		float endurance = 1.0f;
		float armorMassScale = 1.0f;
		float engineTechnology = 1.0f;
		float moduleClearance = 0.12f;
		float hardSurfaceBias = 0.70f;
		WeaponLayoutSettings weaponLayout{};
		bool operator==(const DesignSettings&) const = default;
	};

	struct MountSettings {
		std::string id;
		gen_types::Point3 position{};
		gen_types::Point3 forward{0.0f, 0.0f, 1.0f};
		// Automatic mounts use this as a per-capability nominal firing direction.
		// A value is fixed (omitted JSON means +Z); null delegates direction to the
		// coverage policy.  `forward` remains the resolved output direction.
		std::optional<gen_types::Point3> requestedFacing = gen_types::Point3{0.0f, 0.0f, 1.0f};
		gen_types::Point3 supportRoot{};
		float turretRadius = 0.25f;
		float barrelRadius = 0.25f;
		float barrelLength = 3.0f;
		float traverseHalfAngleDegrees = 45.0f;
		float supportWidth = 0.52f;
		float supportHeight = 0.38f;
		float socketHeight = 0.20f;
		bool operator==(const MountSettings&) const = default;
	};

	struct Settings {
		std::string id;
		std::string modelPath;
		std::uint32_t seed = 4101u;
		Dimensions dimensions{};
		HullSettings hull{};
		WingSettings wings{};
		CockpitSettings cockpit{};
		std::vector<EngineSettings> engines;
		std::vector<MountSettings> mounts;
		LayoutSettings layout{};
		DesignSettings design{};
		WearSettings wear{};
		MaterialSettings material{};
		MountAttachmentSettings mountAttachment{};
		int cylinderSegments = 12;
		int textureWidth = 2048;
		int textureHeight = 2048;
		float armorDepth = 0.10f;
		bool operator==(const Settings&) const = default;
	};

	struct Bounds {
		gen_types::Point3 minimum{};
		gen_types::Point3 maximum{};
		float maximumRadius = 0.0f;
		bool operator==(const Bounds&) const = default;
	};

	struct MountReport {
		std::string id;
		bool structurallyConnected = false;
		float supportThickness = 0.0f;
		float minimumClearance = 0.0f;
		float parentCollisionClearance = 0.0f;
		gen_types::Point3 attachmentPoint{};
		gen_types::Point3 resolvedForward{0.0f, 0.0f, 1.0f};
		float facingErrorDegrees = 0.0f;
		float takeoffAngleDegrees = 0.0f;
		float supportLength = 0.0f;
		bool directBlister = false;
		bool operator==(const MountReport&) const = default;
	};

	struct DesignMetrics {
		PropulsionLayout selectedLayout = PropulsionLayout::Auto;
		float massProxy = 0.0f;
		float weaponBurden = 0.0f;
		float magazineVolume = 0.0f;
		float fuelVolume = 0.0f;
		float engineVolume = 0.0f;
		float reactorVolume = 0.0f;
		float requiredThrust = 0.0f;
		float availableThrust = 0.0f;
		gen_types::Point3 massCenter{};
		gen_types::Point3 thrustCenter{};
		bool operator==(const DesignMetrics&) const = default;
	};

	struct SystemDetailReport {
		std::size_t fuelHousings = 0u;
		std::size_t feedTrunks = 0u;
		std::size_t reactorShields = 0u;
		std::size_t radiatorPanels = 0u;
		std::size_t serviceAccessPanels = 0u;
		bool operator==(const SystemDetailReport&) const = default;
	};

	struct MaterialDetailReport {
		float normalSlopeMean = 0.0f;
		float normalSlopeP95 = 0.0f;
		float normalSlopeMaximum = 0.0f;
		float secondaryCoverage = 0.0f;
		float accentCoverage = 0.0f;
		float thermalCoverage = 0.0f;
		bool operator==(const MaterialDetailReport&) const = default;
	};

	struct GeneratedShip {
		gen_types::AssetData asset;
		Bounds bounds;
		std::vector<MountReport> mounts;
		std::vector<EngineSettings> resolvedEngines;
		std::vector<int> resolvedNozzleCells;
		std::vector<MountSettings> resolvedMounts;
		DesignMetrics design;
		SystemDetailReport systemDetails;
		MaterialDetailReport materialDetails;
		std::uint64_t settingsFingerprint = 0;
	};

	GeneratedShip generate(const Settings& settings);
	[[maybe_unused]] GeneratedShip generateLegacy(const Settings& settings);
	std::uint64_t fingerprint(const Settings& settings);
} // namespace gen_model::spaceship

#endif // GEN_MODEL_SPACESHIP_GENERATOR_HPP
