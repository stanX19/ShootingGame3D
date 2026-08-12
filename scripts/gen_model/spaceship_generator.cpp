#include "spaceship_generator.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "spaceship_mesh.hpp"
#include "spaceship_design.hpp"
#include "spaceship_envelope.hpp"
#include "spaceship_weapon_layout.hpp"

namespace {
	constexpr float PI = 3.14159265358979323846f;
	constexpr float DEGREES_TO_RADIANS = PI / 180.0f;
	constexpr float EPSILON = 0.00001f;
	constexpr float FIRING_CLEARANCE_MARGIN = 0.02f;
	constexpr int FIRING_CONE_AZIMUTH_SAMPLES = 64;
	constexpr std::array<float, 8> FIRING_CONE_RINGS{
		0.125f, 0.25f, 0.375f, 0.50f, 0.625f, 0.75f, 0.875f, 1.0f
	};

	using Point3 = gen_model::gen_types::Point3;
	using Surface = gen_model::spaceship::detail::Surface;
	using MeshBuilder = gen_model::spaceship::detail::MeshBuilder;
	using TaggedMesh = gen_model::spaceship::detail::TaggedMesh;
	using PlacementMode = gen_model::spaceship::PlacementMode;
	using BatteryStyle = gen_model::spaceship::BatteryStyle;
	using PropulsionLayout = gen_model::spaceship::PropulsionLayout;

	struct HullSection {
		float z;
		float halfWidth;
		float top;
		float bottom;
	};

	struct SurfaceCandidate {
		Point3 point{};
		Point3 normal{};
		Point3 tangent{};
	};

	struct SurfaceAttachment {
		Point3 surfacePoint{};
		Point3 surfaceNormal{};
		Point3 surfaceTangent{};
		Point3 lowerTangent{};
		Point3 blisterBase{};
		Point3 blisterNormal{};
		float takeoffAngleDegrees = 0.0f;
		float supportLength = 0.0f;
		bool directBlister = false;
		bool onGeneratedSurface = false;
	};

	float symmetrySnap(float value) {
		// Mount rings are offline geometry; a 1 mm quantization keeps reflected
		// vertices bit-stable while remaining far below the visual/collision scale.
		const float magnitude = std::round(std::abs(value) * 1000.0f) / 1000.0f;
		return std::copysign(magnitude, value);
	}

	Point3 symmetrySnapPoint(Point3 point) {
		point.x = symmetrySnap(point.x);
		point.y = symmetrySnap(point.y);
		point.z = symmetrySnap(point.z);
		return point;
	}

	bool finite(float value) {
		return std::isfinite(value);
	}

	bool finite(Point3 point) {
		return finite(point.x) && finite(point.y) && finite(point.z);
	}

	float lengthSquared(Point3 point) {
		return gen_model::gen_types::dot(point, point);
	}

	float length(Point3 point) {
		return std::sqrt(lengthSquared(point));
	}

	float smoothStep(float amount) {
		amount = std::clamp(amount, 0.0f, 1.0f);
		return amount * amount * (3.0f - 2.0f * amount);
	}

	float smoothRange(float lower, float upper, float value) {
		if (upper <= lower)
			return value >= upper ? 1.0f : 0.0f;
		return smoothStep((value - lower) / (upper - lower));
	}

	float gaussianBand(float distance, float width) {
		if (width <= 0.0f)
			return 0.0f;
		const float normalized = distance / width;
		return std::exp(-normalized * normalized * 2.0f);
	}

	bool isFighterArchetype(const gen_model::spaceship::Settings& settings) {
		return settings.layout.archetype == "patrol_fighter"
			|| settings.layout.archetype == "multirole"
			|| settings.layout.archetype == "heavy_fighter"
			|| settings.layout.archetype == "interceptor";
	}

	float wingRootTop(const gen_model::spaceship::Settings& settings) {
		if (!isFighterArchetype(settings))
			return settings.hull.height * 0.13f + settings.hull.crown * 0.72f;
		// The inboard wing is the pressure-shell shoulder.  Raising its root into
		// the hull side gives the hardpoint a real spar path instead of a pylon.
		return std::max(
			settings.wings.topY + 0.04f,
			settings.hull.height * 0.13f + settings.hull.crown * 0.40f
		);
	}

	float wingRootBottom(const gen_model::spaceship::Settings& settings) {
		if (!isFighterArchetype(settings))
			return -settings.hull.height * 0.34f - settings.hull.keel * 0.18f;
		// The inboard airfoil is part of the pressure shell, but it is not a
		// vertical slab.  Keep its lower skin close to the fuselage belly so the
		// wing root reads as a blended shoulder in side view.
		return std::max(
			settings.wings.bottomY + settings.hull.height * 0.16f,
			wingRootTop(settings) - settings.hull.height * 0.24f
		);
	}

	void validateSettings(const gen_model::spaceship::Settings& settings) {
		if (settings.id.empty())
			throw std::invalid_argument("Spaceship id cannot be empty");
		if (settings.modelPath.empty())
			throw std::invalid_argument("Spaceship modelPath cannot be empty");
		if (!finite(settings.dimensions.width) || !finite(settings.dimensions.height) || !finite(settings.dimensions.length)
			|| settings.dimensions.width <= 0.0f || settings.dimensions.height <= 0.0f || settings.dimensions.length <= 0.0f) {
			throw std::invalid_argument("Spaceship dimensions must be finite and positive");
		}
		if (settings.hull.width <= 0.0f || settings.hull.height <= 0.0f || settings.hull.length <= 0.0f
			|| settings.hull.width > settings.dimensions.width
			|| settings.hull.height > settings.dimensions.height
			|| settings.hull.length > settings.dimensions.length) {
			throw std::invalid_argument("Spaceship hull dimensions must be positive and inside the requested core dimensions");
		}
		if (settings.hull.noseSharpness <= 0.0f || settings.hull.noseSharpness > 1.0f
			|| settings.hull.rearTaper <= 0.0f || settings.hull.rearTaper > 1.0f) {
			throw std::invalid_argument("Spaceship hull taper values must be in (0, 1]");
		}
		if (settings.wings.halfSpan <= settings.wings.rootX || settings.wings.rootX <= 0.0f
			|| settings.wings.topY <= settings.wings.bottomY || settings.wings.shoulderWidth <= 0.0f) {
			throw std::invalid_argument("Spaceship wing layout is invalid");
		}
		if (!finite(settings.cockpit.center) || !finite(settings.cockpit.size)
			|| settings.cockpit.size.x <= 0.0f || settings.cockpit.size.y <= 0.0f || settings.cockpit.size.z <= 0.0f) {
			throw std::invalid_argument("Spaceship cockpit settings are invalid");
		}
		if (settings.layout.archetype.empty() || settings.layout.crew < 0
			|| settings.layout.primarySpineWidth <= 0.0f
			|| settings.layout.reactorRadius <= 0.0f
			|| settings.layout.serviceBayLength <= 0.0f
			|| settings.layout.radiatorScale < 0.0f
			|| settings.layout.weaponDeckCantDegrees < 0.0f
			|| settings.layout.weaponDeckCantDegrees > 60.0f) {
			throw std::invalid_argument("Spaceship internal layout settings are invalid");
		}
		const auto& attachment = settings.mountAttachment;
		if (!finite(attachment.preferredTakeoffAngleDegrees)
			|| !finite(attachment.minimumTakeoffAngleDegrees)
			|| !finite(attachment.maximumTakeoffAngleDegrees)
			|| attachment.minimumTakeoffAngleDegrees < 0.0f
			|| attachment.minimumTakeoffAngleDegrees > attachment.preferredTakeoffAngleDegrees
			|| attachment.preferredTakeoffAngleDegrees > attachment.maximumTakeoffAngleDegrees
			|| attachment.maximumTakeoffAngleDegrees > 85.0f
			|| !finite(attachment.directBlisterGapScale) || attachment.directBlisterGapScale <= 0.0f
			|| !finite(attachment.distanceWeight) || attachment.distanceWeight < 0.0f
			|| !finite(attachment.angleWeight) || attachment.angleWeight < 0.0f
			|| attachment.distanceWeight + attachment.angleWeight <= 0.0f
			|| !finite(attachment.blisterRadiusScale) || attachment.blisterRadiusScale < 1.0f) {
			throw std::invalid_argument("Spaceship mount attachment policy is invalid");
		}
		if (settings.engines.empty())
			throw std::invalid_argument("Spaceship must have at least one engine");
		for (const auto& engine : settings.engines) {
			if (!finite(engine.center) || engine.radius <= 0.0f || engine.length <= 0.0f || engine.nozzleDepth <= 0.0f)
				throw std::invalid_argument("Spaceship engine settings are invalid");
		}
		if (settings.mounts.empty())
			throw std::invalid_argument("Spaceship must have at least one mount");
		for (const auto& mount : settings.mounts) {
			if (mount.id.empty() || !finite(mount.position) || !finite(mount.forward) || !finite(mount.supportRoot))
				throw std::invalid_argument("Spaceship mount identity and vectors must be finite");
			if (lengthSquared(mount.forward) <= EPSILON)
				throw std::invalid_argument("Spaceship mount forward direction cannot be zero");
			if (mount.requestedFacing.has_value()
				&& (!finite(*mount.requestedFacing) || lengthSquared(*mount.requestedFacing) <= EPSILON))
				throw std::invalid_argument("Spaceship mount facing direction cannot be zero or non-finite");
			if (mount.turretRadius <= 0.0f || mount.barrelRadius <= 0.0f
				|| mount.barrelLength <= mount.turretRadius * 10.0f) {
				throw std::invalid_argument("Spaceship mount must use positive radii and a barrel longer than 10 turret radii");
			}
			if (mount.traverseHalfAngleDegrees < 0.0f || mount.traverseHalfAngleDegrees > 80.0f)
				throw std::invalid_argument("Spaceship mount traverse half-angle must be between 0 and 80 degrees");
			const float minimumSupport = mount.turretRadius * 1.5f;
			if (mount.supportWidth < minimumSupport || mount.supportHeight < minimumSupport)
				throw std::invalid_argument("Spaceship mount support is thinner than 1.5 turret radii");
			if (mount.socketHeight <= 0.0f)
				throw std::invalid_argument("Spaceship socket height must be positive");
		}
		if (settings.cylinderSegments < 6 || settings.cylinderSegments > 64)
			throw std::invalid_argument("Spaceship cylinderSegments must be between 6 and 64");
		if (settings.textureWidth < 2 || settings.textureWidth > 4096
			|| settings.textureHeight < 2 || settings.textureHeight > 4096) {
			throw std::invalid_argument("Spaceship texture dimensions must be between 2 and 4096");
		}
		if (settings.wear.paintLoss < 0.0f || settings.wear.paintLoss > 1.0f
			|| settings.wear.oxidation < 0.0f || settings.wear.oxidation > 1.0f
			|| settings.wear.heatStaining < 0.0f || settings.wear.heatStaining > 1.0f
			|| settings.wear.repairPanels < 0 || settings.wear.repairPanels > 32) {
			throw std::invalid_argument("Spaceship wear settings are outside supported bounds");
		}
		if (!finite(settings.material.secondaryCoverage)
			|| !finite(settings.material.accentCoverage)
			|| !finite(settings.material.normalStrength)
			|| !finite(settings.material.detailScale)
			|| settings.material.secondaryCoverage < 0.0f
			|| settings.material.secondaryCoverage > 0.75f
			|| settings.material.accentCoverage < 0.01f
			|| settings.material.accentCoverage > 0.30f
			|| settings.material.normalStrength < 0.04f
			|| settings.material.normalStrength > 0.35f
			|| settings.material.detailScale < 0.50f
			|| settings.material.detailScale > 2.0f) {
			throw std::invalid_argument("Spaceship material design settings are outside supported bounds");
		}
		if (!finite(settings.design.targetAcceleration)
			|| !finite(settings.design.endurance)
			|| !finite(settings.design.armorMassScale)
			|| !finite(settings.design.engineTechnology)
			|| !finite(settings.design.moduleClearance)
			|| !finite(settings.design.hardSurfaceBias)
			|| settings.design.targetAcceleration <= 0.0f
			|| settings.design.endurance <= 0.0f
			|| settings.design.armorMassScale <= 0.0f
			|| settings.design.engineTechnology <= 0.0f
			|| settings.design.moduleClearance < 0.0f
			|| settings.design.hardSurfaceBias < 0.0f
			|| settings.design.hardSurfaceBias > 1.0f
			|| !finite(settings.design.weaponLayout.minimumSeparationScale)
			|| settings.design.weaponLayout.minimumSeparationScale < 1.0f) {
			throw std::invalid_argument("Spaceship design settings are invalid");
		}
	}

	std::array<Point3, 14> crossSection(const HullSection& section, const gen_model::spaceship::HullSettings& hull) {
		const float upperShoulder = section.top + hull.crown * 0.52f;
		const float lowerShoulder = section.bottom - hull.keel * 0.48f;
		return {
			Point3{0.0f, section.top + hull.crown, section.z},
			Point3{section.halfWidth * 0.42f, upperShoulder, section.z},
			Point3{section.halfWidth * 0.78f, section.top * 0.54f, section.z},
			Point3{section.halfWidth, section.top * 0.08f, section.z},
			Point3{section.halfWidth * 0.90f, section.bottom * 0.30f, section.z},
			Point3{section.halfWidth * 0.62f, lowerShoulder, section.z},
			Point3{section.halfWidth * 0.22f, section.bottom * 0.94f, section.z},
			Point3{0.0f, section.bottom - hull.keel, section.z},
			Point3{-section.halfWidth * 0.22f, section.bottom * 0.94f, section.z},
			Point3{-section.halfWidth * 0.62f, lowerShoulder, section.z},
			Point3{-section.halfWidth * 0.90f, section.bottom * 0.30f, section.z},
			Point3{-section.halfWidth, section.top * 0.08f, section.z},
			Point3{-section.halfWidth * 0.78f, section.top * 0.54f, section.z},
			Point3{-section.halfWidth * 0.42f, upperShoulder, section.z}
		};
	}

	std::array<HullSection, 9> makeHullSections(const gen_model::spaceship::Settings& settings) {
		const float halfLength = settings.hull.length * 0.5f;
		const float halfWidth = settings.hull.width * 0.5f;
		const float top = settings.hull.height * 0.52f;
		const float bottom = -settings.hull.height * 0.48f;
		const bool fighter = isFighterArchetype(settings);
		return {
			HullSection{halfLength, halfWidth * 0.035f, top * 0.10f, bottom * 0.10f},
			HullSection{halfLength * 0.82f, halfWidth * (0.30f + settings.hull.noseSharpness * 0.24f), top * 0.46f, bottom * 0.40f},
			HullSection{halfLength * 0.58f, halfWidth * 0.72f, top * 0.76f, bottom * 0.68f},
			HullSection{halfLength * 0.30f, halfWidth * 0.92f, top * 0.92f, bottom * 0.88f},
			HullSection{0.0f, halfWidth, top, bottom},
			HullSection{-halfLength * 0.34f, halfWidth * 0.96f, top * 0.84f, bottom * 0.86f},
			HullSection{
				-halfLength * 0.62f,
				halfWidth * (fighter ? 0.78f : 0.88f),
				top * (fighter ? 0.54f : 0.66f),
				bottom * (fighter ? 0.58f : 0.70f)
			},
			HullSection{
				-halfLength * 0.84f,
				halfWidth * (fighter ? 0.56f : 0.72f),
				top * (fighter ? 0.30f : 0.44f),
				bottom * (fighter ? 0.34f : 0.48f)
			},
			HullSection{
				-halfLength,
				halfWidth * settings.hull.rearTaper * (fighter ? 0.72f : 1.0f),
				top * (fighter ? 0.12f : 0.20f),
				bottom * (fighter ? 0.16f : 0.24f)
			}
		};
	}

	void addFacetedNacelle(
		MeshBuilder& builder,
		const gen_model::spaceship::EngineSettings& engine,
		int segments,
		Surface surface
	) {
		const int ringSegments = std::clamp(segments, 8, 16);
		// Leave the nozzle depth in front of the nacelle's rear pressure cap.  A
		// cap exactly coplanar with the nozzle mouths creates a hidden disk between
		// cells; a short inset keeps the housing closed while the exhaust throat is
		// owned exclusively by the nozzle/collar geometry.
		const float rearInset = std::min(
			engine.length * 0.18f,
			engine.nozzleDepth * 0.42f
		);
		const std::array<float, 5> stations{
			-0.5f + rearInset / std::max(engine.length, EPSILON),
			-0.28f, 0.10f, 0.42f, 0.5f
		};
		const std::array<float, 5> scales{0.62f, 0.94f, 1.0f, 0.88f, 0.52f};
		std::vector<std::vector<Point3>> rings;
		rings.reserve(stations.size());
		for (std::size_t station = 0; station < stations.size(); ++station) {
			std::vector<Point3> ring;
			ring.reserve(static_cast<std::size_t>(ringSegments));
			const float z = engine.center.z + stations[station] * engine.length;
			for (int segment = 0; segment < ringSegments; ++segment) {
				const float angle = 2.0f * PI * static_cast<float>(segment) / static_cast<float>(ringSegments);
				ring.push_back({
					engine.center.x + std::cos(angle) * engine.radius * scales[station],
					engine.center.y + std::sin(angle) * engine.radius * scales[station] * 0.72f,
					z
				});
			}
			rings.push_back(std::move(ring));
		}
		builder.addClosedLoft(rings, surface, surface, surface);
	}

	void addHull(MeshBuilder& builder, const gen_model::spaceship::Settings& settings) {
		const auto sections = makeHullSections(settings);
		std::vector<std::vector<Point3>> rings;
		rings.reserve(sections.size());
		for (const HullSection& section : sections) {
			const auto ring = crossSection(section, settings.hull);
			rings.emplace_back(ring.begin(), ring.end());
		}
		builder.addClosedLoft(
			rings,
			Surface::Armor,
			Surface::Armor,
			Surface::Structure
		);
	}

	struct WingStation {
		float x;
		float frontZ;
		float rearZ;
		float top;
		float bottom;
		float camber;
	};

	std::array<Point3, 12> wingRing(const WingStation& station, float side) {
		const float chord = station.rearZ - station.frontZ;
		const auto chordAt = [&](float amount) {
			return station.frontZ + chord * amount;
		};
		const float topRidge = station.top + station.camber;
		const float bottomRidge = station.bottom + station.camber * 0.30f;
		const float leadingEdge = (station.top + station.bottom) * 0.50f;
		const float trailingEdge = leadingEdge;
		const float edgeHalfThickness = (station.top - station.bottom) * 0.12f;
		return {
			Point3{side * station.x, leadingEdge + edgeHalfThickness, station.frontZ},
			Point3{side * station.x, station.top + station.camber * 0.50f, chordAt(0.12f)},
			Point3{side * station.x, station.top + station.camber * 0.86f, chordAt(0.30f)},
			Point3{side * station.x, topRidge, chordAt(0.50f)},
			Point3{side * station.x, station.top + station.camber * 0.70f, chordAt(0.70f)},
			Point3{side * station.x, trailingEdge + edgeHalfThickness, station.rearZ},
			Point3{side * station.x, trailingEdge - edgeHalfThickness, station.rearZ},
			Point3{side * station.x, station.bottom + station.camber * 0.18f, chordAt(0.70f)},
			Point3{side * station.x, bottomRidge, chordAt(0.50f)},
			Point3{side * station.x, station.bottom + station.camber * 0.22f, chordAt(0.30f)},
			Point3{side * station.x, station.bottom + station.camber * 0.10f, chordAt(0.12f)},
			Point3{side * station.x, leadingEdge - edgeHalfThickness, station.frontZ}
		};
	}

	Point3 wingSectionExpectedNormal(Point3 radialReference, std::size_t section) {
		// The two sections immediately around the leading edge must face the nose;
		// together they close the upper shoulder and the wraparound lip, preventing
		// a front-view gap between the upper and lower wing skins.  The opposite
		// section closes the trailing edge and must face aft.  A radial reference
		// is reliable for the crowned surfaces between them but points along the
		// wrong side of these nearly vertical edge faces.
		if (section == 0u || section == 11u)
			return {0.0f, 0.0f, 1.0f};
		if (section == 5u)
			return {0.0f, 0.0f, -1.0f};
		return radialReference;
	}

	std::vector<WingStation> makeWingStations(const gen_model::spaceship::Settings& settings) {
		const float span = settings.wings.halfSpan;
		const float rootX = isFighterArchetype(settings)
			? std::max(settings.wings.rootX * 0.75f, settings.hull.width * 0.28f)
			: std::max(settings.wings.rootX, settings.hull.width * 0.42f);
		const bool broadAirframe = settings.layout.archetype == "siege_gunship"
			|| settings.layout.archetype == "carrier";
		const float rootTop = broadAirframe
			? settings.hull.height * 0.13f + settings.hull.crown * 0.72f
			: wingRootTop(settings);
		const float rootBottom = wingRootBottom(settings);
		const float tipTop = settings.wings.topY + 0.05f;
		const float tipBottom = settings.wings.bottomY + 0.20f;
		const float rootCamber = settings.layout.archetype == "interceptor" ? 0.20f : 0.16f;
		const float tipCamber = settings.layout.archetype == "interceptor" ? 0.08f : 0.06f;
		const std::array<float, 7> stationAmounts{0.0f, 0.16f, 0.32f, 0.50f, 0.68f, 0.84f, 1.0f};
		std::vector<WingStation> stations;
		stations.reserve(stationAmounts.size());
		for (const float amount : stationAmounts) {
			const float eased = smoothStep(amount);
			stations.push_back({
				rootX + (span - rootX) * amount,
				settings.wings.rootFrontZ + (settings.wings.tipFrontZ - settings.wings.rootFrontZ) * eased,
				settings.wings.rootRearZ + (settings.wings.tipRearZ - settings.wings.rootRearZ) * eased,
				rootTop + (tipTop - rootTop) * eased,
				rootBottom + (tipBottom - rootBottom) * eased,
				rootCamber + (tipCamber - rootCamber) * eased
			});
		}
		return stations;
	}

	// A wing is an airfoil-like load path, not a flat plate.  Each station has a
	// shallow crowned section with a tapered edge; the root dies into the hull and
	// the tip tapers to a knife edge.  This keeps hardpoints and engine fairings
	// visually grown from the same lifting surface instead of bolted onto it.
	void addWingHalf(
		MeshBuilder& builder,
		const std::vector<WingStation>& stations,
		float side,
		Surface surface
	) {
		if (stations.size() < 2)
			throw std::invalid_argument("A spaceship wing needs at least two loft stations");
		std::vector<std::vector<Point3>> rings;
		rings.reserve(stations.size());
		for (const WingStation& station : stations) {
			const auto ring = wingRing(station, side);
			rings.emplace_back(ring.begin(), ring.end());
		}
		// The root cap remains embedded in the pressure hull, but sealing it keeps
		// the wing a mechanically checkable closed load shell.
		builder.addClosedLoft(rings, surface, surface, surface);
	}

	void addWings(MeshBuilder& builder, const gen_model::spaceship::Settings& settings) {
		const auto stations = makeWingStations(settings);
		addWingHalf(builder, stations, 1.0f, Surface::Armor);
		addWingHalf(builder, stations, -1.0f, Surface::Armor);
	}

	void appendTriangleSamples(
		std::vector<SurfaceCandidate>& candidates,
		Point3 a,
		Point3 b,
		Point3 c,
		Point3 expectedNormal,
		const gen_model::spaceship::MountSettings& mount
	) {
		Point3 normal = gen_model::gen_types::cross(b - a, c - a);
		if (lengthSquared(normal) <= EPSILON)
			return;
		normal = gen_model::gen_types::normalize(normal);
		if (gen_model::gen_types::dot(normal, expectedNormal) < 0.0f)
			normal = normal * -1.0f;
		const Point3 center = (a + b + c) * (1.0f / 3.0f);
		if (gen_model::gen_types::dot(normal, mount.position - center) <= 0.0f)
			return;
		Point3 tangent = b - a;
		if (lengthSquared(tangent) <= EPSILON)
			tangent = c - a;
		tangent = gen_model::gen_types::normalize(tangent);
		constexpr int subdivisions = 6;
		for (int row = 0; row <= subdivisions; ++row) {
			for (int column = 0; column <= subdivisions - row; ++column) {
				const float bWeight = static_cast<float>(row) / static_cast<float>(subdivisions);
				const float cWeight = static_cast<float>(column) / static_cast<float>(subdivisions);
				const float aWeight = 1.0f - bWeight - cWeight;
				const Point3 point = a * aWeight + b * bWeight + c * cWeight;
				if (std::abs(mount.position.x) > EPSILON && point.x * mount.position.x < -EPSILON)
					continue;
				candidates.push_back({point, normal, tangent});
			}
		}
	}

	void appendQuadSamples(
		std::vector<SurfaceCandidate>& candidates,
		Point3 a,
		Point3 b,
		Point3 c,
		Point3 d,
		Point3 expectedNormal,
		const gen_model::spaceship::MountSettings& mount
	) {
		// MeshBuilder::addQuad uses this same diagonal (and may reverse winding),
		// so every candidate remains on an emitted triangle plane.
		appendTriangleSamples(candidates, a, b, c, expectedNormal, mount);
		appendTriangleSamples(candidates, a, c, d, expectedNormal, mount);
	}

	SurfaceCandidate externalRailDeckCandidate(
		const gen_model::spaceship::MountSettings& mount
	) {
		Point3 normal = mount.position - mount.supportRoot;
		if (lengthSquared(normal) <= EPSILON)
			throw std::invalid_argument(
				"External spaceship mount has no rail-deck offset: " + mount.id
			);
		normal = gen_model::gen_types::normalize(normal);
		Point3 tangent = std::abs(normal.z) < 0.90f
			? gen_model::gen_types::cross(normal, Point3{0.0f, 0.0f, 1.0f})
			: gen_model::gen_types::cross(normal, Point3{1.0f, 0.0f, 0.0f});
		if (lengthSquared(tangent) <= EPSILON)
			throw std::logic_error("External spaceship mount rail has no tangent: " + mount.id);
		return {
			mount.supportRoot,
			normal,
			gen_model::gen_types::normalize(tangent)
		};
	}

	std::vector<SurfaceCandidate> sampleGeneratedSurfaces(
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::MountSettings& mount
	) {
		std::vector<SurfaceCandidate> candidates;
		candidates.reserve(2600);
		if (!isFighterArchetype(settings)) {
			const auto sections = makeHullSections(settings);
			std::array<std::array<Point3, 14>, 9> rings{};
			for (std::size_t index = 0; index < sections.size(); ++index)
				rings[index] = crossSection(sections[index], settings.hull);
			for (std::size_t station = 0; station + 1 < rings.size(); ++station) {
				const float centerZ = (sections[station].z + sections[station + 1].z) * 0.5f;
				for (std::size_t side = 0; side < rings[station].size(); ++side) {
					const std::size_t next = (side + 1) % rings[station].size();
					const Point3 faceCenter = (
						rings[station][side] + rings[station][next]
						+ rings[station + 1][next] + rings[station + 1][side]
					) * 0.25f;
					appendQuadSamples(
						candidates,
						rings[station][side], rings[station + 1][side],
						rings[station + 1][next], rings[station][next],
						faceCenter - Point3{0.0f, 0.0f, centerZ}, mount
					);
				}
			}
		}
		const auto stations = makeWingStations(settings);
		for (const float side : {1.0f, -1.0f}) {
			if (std::abs(mount.position.x) > EPSILON && side * mount.position.x < 0.0f)
				continue;
			std::vector<std::array<Point3, 12>> rings;
			rings.reserve(stations.size());
			for (const WingStation& station : stations)
				rings.push_back(wingRing(station, side));
			for (std::size_t station = 0; station + 1 < rings.size(); ++station) {
				const Point3 centerLine = (rings[station][0] + rings[station + 1][0]) * 0.5f;
				for (std::size_t section = 0; section < rings[station].size(); ++section) {
					const std::size_t next = (section + 1) % rings[station].size();
					const Point3 faceCenter = (
						rings[station][section] + rings[station][next]
						+ rings[station + 1][next] + rings[station + 1][section]
					) * 0.25f;
					appendQuadSamples(
						candidates,
						rings[station][section], rings[station + 1][section],
						rings[station + 1][next], rings[station][next],
						wingSectionExpectedNormal(faceCenter - centerLine, section), mount
					);
				}
			}
		}
		return candidates;
	}

	SurfaceAttachment evaluateCandidate(
		const SurfaceCandidate& candidate,
		const gen_model::spaceship::MountSettings& mount
	) {
		SurfaceAttachment result;
		result.surfacePoint = candidate.point;
		result.surfaceNormal = candidate.normal;
		result.lowerTangent = mount.position - candidate.normal * (mount.turretRadius * 0.96f);
		const Point3 delta = result.lowerTangent - result.surfacePoint;
		result.supportLength = length(delta);
		const Point3 direction = result.supportLength > EPSILON
			? delta * (1.0f / result.supportLength)
			: candidate.normal;
		result.takeoffAngleDegrees = std::asin(std::clamp(
			gen_model::gen_types::dot(direction, candidate.normal), 0.0f, 1.0f
		)) / DEGREES_TO_RADIANS;
		const Point3 planar = direction - candidate.normal
			* gen_model::gen_types::dot(direction, candidate.normal);
		result.surfaceTangent = lengthSquared(planar) > EPSILON
			? gen_model::gen_types::normalize(planar)
			: candidate.tangent;
		result.onGeneratedSurface = true;
		return result;
	}

	bool deterministicTieBreak(const SurfaceAttachment& left, const SurfaceAttachment& right) {
		const std::array<float, 4> leftKey{
			std::abs(left.surfacePoint.x), std::abs(left.surfacePoint.y),
			std::abs(left.surfacePoint.z), left.surfacePoint.z
		};
		const std::array<float, 4> rightKey{
			std::abs(right.surfacePoint.x), std::abs(right.surfacePoint.y),
			std::abs(right.surfacePoint.z), right.surfacePoint.z
		};
		return leftKey < rightKey;
	}

	SurfaceAttachment chooseAttachment(
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::MountSettings& mount
	) {
		// Solve one side in canonical (+X) space and reflect the result for the
		// opposite wing.  This removes floating-point/tie-order asymmetry from the
		// optimizer and guarantees a truly mirrored hardpoint battery.
		if (mount.position.x < -EPSILON) {
			auto mirroredMount = mount;
			mirroredMount.position.x = -mirroredMount.position.x;
			mirroredMount.forward.x = -mirroredMount.forward.x;
			mirroredMount.supportRoot.x = -mirroredMount.supportRoot.x;
			SurfaceAttachment result = chooseAttachment(settings, mirroredMount);
			auto reflect = [](Point3 point) {
				point.x = -point.x;
				return point;
			};
			result.surfacePoint = reflect(result.surfacePoint);
			result.surfaceNormal = reflect(result.surfaceNormal);
			result.surfaceTangent = reflect(result.surfaceTangent);
			result.lowerTangent = reflect(result.lowerTangent);
			result.blisterBase = reflect(result.blisterBase);
			result.blisterNormal = reflect(result.blisterNormal);
			return result;
		}
		if (settings.design.weaponLayout.placement == PlacementMode::Auto
			&& settings.design.weaponLayout.batteryStyle == BatteryStyle::External) {
			SurfaceAttachment result = evaluateCandidate(
				externalRailDeckCandidate(mount), mount
			);
			result.directBlister = true;
			result.blisterBase = result.surfacePoint;
			result.blisterNormal = result.surfaceNormal;
			return result;
		}
		const auto candidates = sampleGeneratedSurfaces(settings, mount);
		if (candidates.empty())
			throw std::invalid_argument("Spaceship mount has no candidate attachment surface: " + mount.id);
		std::vector<SurfaceAttachment> evaluated;
		evaluated.reserve(candidates.size());
		for (const SurfaceCandidate& candidate : candidates)
			evaluated.push_back(evaluateCandidate(candidate, mount));
		const Point3 normalizedForward = gen_model::gen_types::normalize(mount.forward);
		const auto outsideMuzzleHalfSpace = [&](const SurfaceAttachment& value) {
			return gen_model::gen_types::dot(
				value.surfacePoint - mount.position,
				normalizedForward
			) <= mount.barrelRadius * 0.20f;
		};
		const bool hasMuzzleSafeCandidate = std::any_of(
			evaluated.begin(), evaluated.end(), outsideMuzzleHalfSpace
		);
		std::size_t nearest = 0;
		while (hasMuzzleSafeCandidate && nearest < evaluated.size()
			&& !outsideMuzzleHalfSpace(evaluated[nearest]))
			++nearest;
		if (nearest == evaluated.size())
			throw std::logic_error("Spaceship mount attachment search lost every candidate");
		for (std::size_t index = nearest + 1u; index < evaluated.size(); ++index) {
			if (hasMuzzleSafeCandidate && !outsideMuzzleHalfSpace(evaluated[index]))
				continue;
			const float delta = evaluated[index].supportLength - evaluated[nearest].supportLength;
			if (delta < -EPSILON || (std::abs(delta) <= EPSILON
				&& deterministicTieBreak(evaluated[index], evaluated[nearest]))) {
				nearest = index;
			}
		}
		const auto& policy = settings.mountAttachment;
		if (evaluated[nearest].supportLength <= policy.directBlisterGapScale * mount.turretRadius) {
			SurfaceAttachment result = evaluated[nearest];
			result.directBlister = true;
			result.blisterBase = result.surfacePoint;
			result.blisterNormal = result.surfaceNormal;
			return result;
		}
		const auto feasible = [&](const SurfaceAttachment& value) {
			return (!hasMuzzleSafeCandidate || outsideMuzzleHalfSpace(value))
				&& value.takeoffAngleDegrees >= policy.minimumTakeoffAngleDegrees
				&& value.takeoffAngleDegrees <= policy.maximumTakeoffAngleDegrees;
		};
		bool hasFeasible = false;
		for (const SurfaceAttachment& value : evaluated)
			hasFeasible = hasFeasible || feasible(value);
		const float angleSpan = std::max(
			policy.maximumTakeoffAngleDegrees - policy.minimumTakeoffAngleDegrees, 1.0f
		);
		std::size_t best = 0;
		float bestScore = std::numeric_limits<float>::max();
		for (std::size_t index = 0; index < evaluated.size(); ++index) {
			const SurfaceAttachment& value = evaluated[index];
			if (hasMuzzleSafeCandidate && !outsideMuzzleHalfSpace(value))
				continue;
			if (hasFeasible && !feasible(value))
				continue;
			const float normalizedLength = value.supportLength / mount.turretRadius;
			const float normalizedAngle = (
				value.takeoffAngleDegrees - policy.preferredTakeoffAngleDegrees
			) / angleSpan;
			float score = policy.distanceWeight * normalizedLength * normalizedLength
				+ policy.angleWeight * normalizedAngle * normalizedAngle;
			if (!hasFeasible) {
				const float outside = value.takeoffAngleDegrees < policy.minimumTakeoffAngleDegrees
					? policy.minimumTakeoffAngleDegrees - value.takeoffAngleDegrees
					: std::max(value.takeoffAngleDegrees - policy.maximumTakeoffAngleDegrees, 0.0f);
				const float penalty = outside / angleSpan;
				score += policy.angleWeight * 4.0f * penalty * penalty;
			}
			const float scoreDelta = score - bestScore;
			if (scoreDelta < -EPSILON
				|| (std::abs(scoreDelta) <= EPSILON
					&& (value.supportLength < evaluated[best].supportLength - EPSILON
						|| (std::abs(value.supportLength - evaluated[best].supportLength) <= EPSILON
							&& deterministicTieBreak(value, evaluated[best]))))) {
				best = index;
				bestScore = score;
			}
		}
		SurfaceAttachment result = evaluated[best];
		const Point3 supportAxis = gen_model::gen_types::normalize(
			result.lowerTangent - result.surfacePoint
		);
		result.blisterNormal = gen_model::gen_types::normalize(
			supportAxis * 0.55f + result.surfaceNormal * 0.45f
		);
		const float blisterHeight = std::max(mount.socketHeight, mount.turretRadius * 0.65f);
		result.blisterBase = result.lowerTangent - result.blisterNormal * blisterHeight;
		return result;
	}

	// Fighters use one pressure/lifting shell instead of a fuselage plus a plate
	// wing.  The longitudinal loft keeps the nose, shoulder, and engine deck on
	// one load path; the wing envelope is blended into the hull section before any
	// cockpit, engine, or hardpoint detail is added.
	void addIntegratedFighterAirframe(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings
	) {
		const auto hullSections = makeHullSections(settings);
		// The original nine stations made the blended shell read as a faceted box
		// wherever the wing joined the pressure hull.  Loft each design interval
		// through several cosine-eased stations so the leading edge, belly, and
		// engine deck all taper as a single aerodynamic surface while keeping the
		// JSON controls unchanged.
		std::vector<HullSection> airframeSections;
		constexpr int longitudinalSubdivisions = 4;
		airframeSections.reserve((hullSections.size() - 1u) * longitudinalSubdivisions + 1u);
		for (std::size_t index = 0; index + 1 < hullSections.size(); ++index) {
			const HullSection& left = hullSections[index];
			const HullSection& right = hullSections[index + 1];
			for (int subdivision = 0; subdivision < longitudinalSubdivisions; ++subdivision) {
				const float amount = static_cast<float>(subdivision) / static_cast<float>(longitudinalSubdivisions);
				const float eased = 0.5f - 0.5f * std::cos(amount * PI);
				airframeSections.push_back({
					left.z + (right.z - left.z) * eased,
					left.halfWidth + (right.halfWidth - left.halfWidth) * eased,
					left.top + (right.top - left.top) * eased,
					left.bottom + (right.bottom - left.bottom) * eased
				});
			}
		}
		airframeSections.push_back(hullSections.back());
		const float span = settings.wings.halfSpan;
		const float rootX = std::max(settings.wings.rootX, settings.hull.width * 0.42f);
		const float midX = rootX + (span - rootX) * (settings.layout.archetype == "interceptor" ? 0.48f : 0.56f);
		const bool broadAirframe = settings.layout.archetype == "siege_gunship"
			|| settings.layout.archetype == "carrier";
		const float rootTop = broadAirframe
			? settings.hull.height * 0.13f + settings.hull.crown * 0.72f
			: wingRootTop(settings);
		const float rootBottom = wingRootBottom(settings);
		const float midTop = settings.wings.topY + (settings.layout.archetype == "interceptor" ? 0.13f : 0.10f);
		const float midBottom = settings.wings.bottomY + 0.08f;
		const float tipTop = settings.wings.topY + 0.08f;
		const float tipBottom = settings.wings.bottomY + 0.14f;
		const float rootCamber = settings.layout.archetype == "interceptor" ? 0.20f : 0.16f;
		const float tipCamber = settings.layout.archetype == "interceptor" ? 0.08f : 0.06f;
		const WingStation root{
			rootX, settings.wings.rootFrontZ, settings.wings.rootRearZ,
			rootTop, rootBottom, rootCamber
		};
		const WingStation mid{
			midX,
			settings.wings.rootFrontZ * 0.38f + settings.wings.tipFrontZ * 0.62f,
			settings.wings.rootRearZ * 0.40f + settings.wings.tipRearZ * 0.60f,
			midTop, midBottom, (rootCamber + tipCamber) * 0.55f
		};
		const WingStation tip{
			span, settings.wings.tipFrontZ, settings.wings.tipRearZ,
			tipTop, tipBottom, tipCamber
		};

		auto interpolate = [](float left, float right, float amount) {
			return left + (right - left) * amount;
		};
		auto wingValueAtX = [&](float x, float WingStation::*member) {
			const float amount = std::clamp((x - root.x) / std::max(span - root.x, EPSILON), 0.0f, 1.0f);
			const float midAmount = std::clamp((mid.x - root.x) / std::max(span - root.x, EPSILON), 0.0f, 1.0f);
			if (amount <= midAmount)
				return interpolate(root.*member, mid.*member, smoothStep(amount / std::max(midAmount, EPSILON)));
			return interpolate(mid.*member, tip.*member, smoothStep((amount - midAmount) / std::max(1.0f - midAmount, EPSILON)));
		};
		auto outerWingAtZ = [&](float z) {
			float outer = 0.0f;
			constexpr int samples = 256;
			for (int sample = 0; sample <= samples; ++sample) {
				const float x = root.x + (span - root.x) * static_cast<float>(sample) / static_cast<float>(samples);
				const float front = wingValueAtX(x, &WingStation::frontZ);
				const float rear = wingValueAtX(x, &WingStation::rearZ);
				if (z >= std::min(front, rear) - 0.0001f && z <= std::max(front, rear) + 0.0001f)
					outer = std::max(outer, x);
			}
			return outer;
		};
		auto hullTopBottomAt = [&](const HullSection& section, float x) {
			const float sectionWidth = std::max(section.halfWidth, EPSILON);
			const float normalized = std::clamp(std::abs(x) / sectionWidth, 0.0f, 1.0f);
			// Use a zero-slope crown at the centerline.  A power of |x| produces a
			// pointed ridge, so the longitudinal airframe quads light as a visible
			// center strip/grid even when their material is continuous.  Cosine falloff
			// keeps the pressure shell smooth through the keel-to-shoulder transition.
			const float cosineFalloff = std::cos(normalized * PI * 0.5f);
			// The player has a wide, low-observable pressure crown.  Flattening the
			// middle of that crown keeps the skin aerodynamic without creating a hard
			// center ridge that reads like a repeated panel/grid seam under light.
			const float crownPower = settings.id == "player" ? 0.45f : 1.0f;
			const float blend = std::pow(std::max(cosineFalloff, 0.0f), crownPower);
			const float sideTop = section.top * 0.08f;
			const float centerTop = section.top + settings.hull.crown;
			const float sideBottom = section.bottom * 0.30f;
			const float centerBottom = section.bottom - settings.hull.keel;
			return std::pair<float, float>{
				sideTop + (centerTop - sideTop) * blend,
				sideBottom + (centerBottom - sideBottom) * blend
			};
		};

		auto shoulderLiftAt = [&](float x, float z) {
			// The curved fairing carries each hardpoint load path now.  Raising the
			// entire loft around every mount created four independent bumps that read
			// as jagged teeth in the pressure skin; keep the underlying airfoil smooth
			// and let the saddle provide the local thickness.
			(void)x;
			(void)z;
			return 0.0f;
		};

		// The pressure skin is a continuous loft, not a 13-strip panel array.  More
		// stations across the crown keep directional lighting from quantizing into a
		// broad center grid while retaining enough hard-surface silhouette control.
		constexpr int crossSectionSamples = 25;
		constexpr int centerIndex = crossSectionSamples / 2;
		std::vector<std::array<Point3, crossSectionSamples>> topRings;
		std::vector<std::array<Point3, crossSectionSamples>> bottomRings;
		topRings.reserve(airframeSections.size());
		bottomRings.reserve(airframeSections.size());
		const bool blendWingsIntoHull = !isFighterArchetype(settings);
		for (const HullSection& section : airframeSections) {
			// Fighters get a clean fuselage loft plus a root-overlapping airfoil below;
			// folding the wing envelope into every fuselage ring caused discontinuous
			// shoulder widths at the swept leading/trailing edges.  Capital ships keep
			// the broad blended envelope because their deck is intentionally continuous.
			const float wingHalfWidth = blendWingsIntoHull ? outerWingAtZ(section.z) : 0.0f;
			const float outerHalfWidth = blendWingsIntoHull
				? std::max(section.halfWidth, wingHalfWidth)
				: section.halfWidth;
			std::array<Point3, crossSectionSamples> topRing{};
			std::array<Point3, crossSectionSamples> bottomRing{};
			for (int index = 0; index < crossSectionSamples; ++index) {
				const float side = static_cast<float>(index - centerIndex) / static_cast<float>(centerIndex);
				const float x = side * outerHalfWidth;
				const auto hullValues = hullTopBottomAt(section, x);
				float topY = hullValues.first;
				float bottomY = hullValues.second;
				if (wingHalfWidth > section.halfWidth + EPSILON && std::abs(x) > section.halfWidth) {
					const float wingX = std::abs(x);
					const float wingTop = wingValueAtX(wingX, &WingStation::top)
						+ wingValueAtX(wingX, &WingStation::camber) * 0.22f;
					const float wingBottom = wingValueAtX(wingX, &WingStation::bottom)
						+ wingValueAtX(wingX, &WingStation::camber) * 0.10f;
					const float blendStart = section.halfWidth * 0.56f;
					const float blend = smoothStep(std::clamp(
						(std::abs(x) - blendStart) / std::max(outerHalfWidth - blendStart, EPSILON),
						0.0f, 1.0f
					));
					topY = interpolate(topY, wingTop, blend);
					bottomY = interpolate(bottomY, wingBottom, blend);
				}
				const float shoulderLift = shoulderLiftAt(x, section.z);
				topY += shoulderLift;
				bottomY += shoulderLift * 0.18f;
				topRing[static_cast<std::size_t>(index)] = {x, topY, section.z};
				bottomRing[static_cast<std::size_t>(index)] = {x, bottomY, section.z};
			}
			topRings.push_back(topRing);
			bottomRings.push_back(bottomRing);
		}

		for (std::size_t station = 0; station + 1 < topRings.size(); ++station) {
			const Point3 centerLine = (topRings[station][centerIndex] + topRings[station + 1][centerIndex]) * 0.5f;
			for (std::size_t index = 0; index + 1 < topRings[station].size(); ++index) {
				// The crown flattens at the centerline, so a radial face-center
				// reference reverses the two center strips.  +Y is the stable
				// outward orientation anchor; addQuad still preserves the local slope.
				builder.addQuad(
					topRings[station][index], topRings[station + 1][index],
					topRings[station + 1][index + 1], topRings[station][index + 1],
					{0.0f, 1.0f, 0.0f}, Surface::Armor
				);
				const Point3 bottomCenter = (
					bottomRings[station][index] + bottomRings[station][index + 1]
					+ bottomRings[station + 1][index + 1] + bottomRings[station + 1][index]
				) * 0.25f;
				builder.addQuad(
					bottomRings[station][index], bottomRings[station][index + 1],
					bottomRings[station + 1][index + 1], bottomRings[station + 1][index],
					bottomCenter - centerLine, Surface::Armor
				);
			}
			for (const int side : {-1, 1}) {
				const std::size_t edge = side < 0 ? 0u : static_cast<std::size_t>(crossSectionSamples - 1);
				const Point3 edgeCenter = (
					topRings[station][edge] + topRings[station + 1][edge]
					+ bottomRings[station + 1][edge] + bottomRings[station][edge]
				) * 0.25f;
				builder.addQuad(
					topRings[station][edge], topRings[station + 1][edge],
					bottomRings[station + 1][edge], bottomRings[station][edge],
					edgeCenter - Point3{
						0.0f,
						0.0f,
						(airframeSections[station].z + airframeSections[station + 1].z) * 0.5f
					},
					Surface::Armor
				);
			}
		}
		for (const std::size_t end : {std::size_t{0}, topRings.size() - 1u}) {
			std::vector<Point3> cap;
			cap.reserve(topRings[end].size() + bottomRings[end].size());
			cap.insert(cap.end(), topRings[end].begin(), topRings[end].end());
			cap.insert(cap.end(), bottomRings[end].rbegin(), bottomRings[end].rend());
			const std::size_t neighbor = end == 0u ? 1u : end - 1u;
			const Point3 endCenter = (
				topRings[end][centerIndex] + bottomRings[end][centerIndex]
			) * 0.5f;
			const Point3 neighborCenter = (
				topRings[neighbor][centerIndex] + bottomRings[neighbor][centerIndex]
			) * 0.5f;
			builder.addConvexCap(
				cap,
				endCenter - neighborCenter,
				Surface::Structure
			);
		}
	}

	void addCockpit(MeshBuilder& builder, const gen_model::spaceship::Settings& settings) {
		// The canopy is a low, faceted pressure blister.  A rectangular beam made the
		// bridge look like a box placed on top of the wing; these lofted rings give it a
		// wind-swept brow and a narrow forward view while leaving armor around the base.
		struct CanopyStation {
			float z;
			float halfWidth;
			float bottom;
			float top;
		};
		const float centerZ = settings.cockpit.center.z;
		const float centerY = settings.cockpit.center.y;
		const float sizeZ = settings.cockpit.size.z;
		const float sizeY = settings.cockpit.size.y;
		const std::array<CanopyStation, 4> stations{
			CanopyStation{centerZ - sizeZ * 0.50f, settings.cockpit.size.x * 0.38f, centerY - sizeY * 0.28f, centerY + sizeY * 0.26f},
			CanopyStation{centerZ - sizeZ * 0.14f, settings.cockpit.size.x * 0.50f, centerY - sizeY * 0.42f, centerY + sizeY * 0.48f},
			CanopyStation{centerZ + sizeZ * 0.28f, settings.cockpit.size.x * 0.36f, centerY - sizeY * 0.30f, centerY + sizeY * 0.40f},
			CanopyStation{centerZ + sizeZ * 0.50f, settings.cockpit.size.x * 0.13f, centerY - sizeY * 0.10f, centerY + sizeY * 0.22f}
		};
		std::vector<std::vector<Point3>> rings;
		rings.reserve(stations.size());
		for (const CanopyStation& station : stations) {
			const float shoulder = station.bottom + (station.top - station.bottom) * 0.62f;
			rings.push_back({
				Point3{-station.halfWidth, station.bottom, station.z},
				Point3{-station.halfWidth * 0.84f, shoulder, station.z},
				Point3{-station.halfWidth * 0.34f, station.top, station.z},
				Point3{station.halfWidth * 0.34f, station.top, station.z},
				Point3{station.halfWidth * 0.84f, shoulder, station.z},
				Point3{station.halfWidth, station.bottom, station.z}
			});
		}
		builder.addClosedLoft(
			rings,
			Surface::Canopy,
			Surface::Canopy,
			Surface::Canopy
		);

		// Keep the viewing pane free of added armor.  A raised prism frame looked like
		// metal pasted onto the glass from the three-quarter and front views; panel
		// seams and normal relief in the canopy atlas carry the small-scale framing
		// detail without introducing a self-occluding solid.
	}

	void addTaperedPlanFairing(
		MeshBuilder& builder,
		Point3 root,
		Point3 tip,
		float rootWidth,
		float tipWidth,
		float rootTop,
		float rootBottom,
		float tipTop,
		float tipBottom,
		Surface surface
	) {
		const Point3 delta{tip.x - root.x, 0.0f, tip.z - root.z};
		const float deltaLength = length(delta);
		if (deltaLength <= EPSILON)
			return;
		const Point3 lateral{-delta.z / deltaLength, 0.0f, delta.x / deltaLength};
		// A perpendicular has two valid signs.  Choose the sign from the side of
		// the airframe so a mirrored fairing is also mirrored in its plan vertices;
		// otherwise both sides can use the same lateral orientation and one wing
		// develops a visibly different leading/trailing edge.
		const float sideSign = std::abs(root.x + tip.x) > EPSILON
			? (root.x + tip.x >= 0.0f ? 1.0f : -1.0f)
			: 1.0f;
		const Point3 mirroredLateral{lateral.x * sideSign, 0.0f, lateral.z * sideSign};
		// Use a shallow rounded longitudinal loft instead of a six-vertex fan.  The
		// same primitive is used for engine fairings, deck doublers, and service
		// spines, so those parts inherit a manufactured leading edge and a tapered
		// trailing edge rather than looking like unrelated boxes.
		const std::array<float, 4> amounts{0.0f, 0.30f, 0.68f, 1.0f};
		std::vector<std::vector<Point3>> rings(amounts.size());
		for (std::size_t station = 0; station < amounts.size(); ++station) {
			const float amount = amounts[station];
			const Point3 center = root + (tip - root) * amount;
			const Point3 centerXZ{center.x, 0.0f, center.z};
			const float width = rootWidth + (tipWidth - rootWidth) * amount;
			const float top = rootTop + (tipTop - rootTop) * amount;
			const float bottom = rootBottom + (tipBottom - rootBottom) * amount;
			const float halfWidth = width * 0.5f;
			const float crown = std::max((top - bottom) * 0.22f, 0.018f) * (1.0f - amount * 0.35f);
			rings[station] = {
				centerXZ - mirroredLateral * halfWidth + Point3{0.0f, top, 0.0f},
				centerXZ - mirroredLateral * (halfWidth * 0.52f) + Point3{0.0f, top + crown * 0.62f, 0.0f},
				centerXZ + Point3{0.0f, top + crown, 0.0f},
				centerXZ + mirroredLateral * (halfWidth * 0.52f) + Point3{0.0f, top + crown * 0.62f, 0.0f},
				centerXZ + mirroredLateral * halfWidth + Point3{0.0f, top, 0.0f},
				centerXZ + mirroredLateral * halfWidth + Point3{0.0f, bottom, 0.0f},
				centerXZ + mirroredLateral * (halfWidth * 0.52f) + Point3{0.0f, bottom - crown * 0.22f, 0.0f},
				centerXZ + Point3{0.0f, bottom - crown * 0.30f, 0.0f},
				centerXZ - mirroredLateral * (halfWidth * 0.52f) + Point3{0.0f, bottom - crown * 0.22f, 0.0f},
				centerXZ - mirroredLateral * halfWidth + Point3{0.0f, bottom, 0.0f}
			};
		}
		builder.addClosedLoft(rings, surface, surface, surface);
	}

	std::vector<Point3> nozzleCellOffsets(int cellCount, float podRadius) {
		if (cellCount <= 1)
			return {{0.0f, 0.0f, 0.0f}};
		std::vector<Point3> result;
		result.reserve(static_cast<std::size_t>(cellCount));
		if (cellCount == 2) {
			// The two-cell throat is a pair of real exhaust apertures, not two
			// overlapping cylinders.  Keep a manufacturing gap between their
			// circular envelopes so the exported rear surface has no coplanar
			// duplicate cap region.
			result.push_back({-podRadius * 0.39f, 0.0f, 0.0f});
			result.push_back({podRadius * 0.39f, 0.0f, 0.0f});
			return result;
		}
		if (cellCount == 5)
			result.push_back({0.0f, 0.0f, 0.0f});
		const int ringCells = cellCount == 5 ? 4 : cellCount;
		const float ringRadius = podRadius * (cellCount <= 4 ? 0.38f : 0.43f);
		for (int index = 0; index < ringCells; ++index) {
			const float angle = PI * 0.5f
				+ 2.0f * PI * static_cast<float>(index) / static_cast<float>(ringCells);
			result.push_back({
				std::cos(angle) * ringRadius,
				std::sin(angle) * ringRadius,
				0.0f
			});
		}
		return result;
	}

	float nozzleCellRadius(float engineRadius, int nozzleCells) {
		return engineRadius * (
			nozzleCells == 1
				? 0.68f
				: (nozzleCells == 2
					? 0.34f
					: (nozzleCells <= 4 ? 0.30f : 0.24f))
		);
	}

	float pointSegmentDistanceSquared2D(Point3 point, Point3 start, Point3 end) {
		const float dx = end.x - start.x;
		const float dy = end.y - start.y;
		const float lengthSquared = dx * dx + dy * dy;
		if (lengthSquared <= EPSILON)
			return (point.x - start.x) * (point.x - start.x)
				+ (point.y - start.y) * (point.y - start.y);
		const float amount = std::clamp(
			((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSquared,
			0.0f,
			1.0f
		);
		const float closestX = start.x + dx * amount;
		const float closestY = start.y + dy * amount;
		return (point.x - closestX) * (point.x - closestX)
			+ (point.y - closestY) * (point.y - closestY);
	}

	float cross2D(Point3 left, Point3 right, Point3 point) {
		return (right.x - left.x) * (point.y - left.y)
			- (right.y - left.y) * (point.x - left.x);
	}

	bool pointInTriangle2D(Point3 point, Point3 a, Point3 b, Point3 c) {
		const float first = cross2D(a, b, point);
		const float second = cross2D(b, c, point);
		const float third = cross2D(c, a, point);
		const bool hasNegative = first < -EPSILON || second < -EPSILON || third < -EPSILON;
		const bool hasPositive = first > EPSILON || second > EPSILON || third > EPSILON;
		return !(hasNegative && hasPositive);
	}

	float triangleDistanceSquared2D(Point3 center, Point3 a, Point3 b, Point3 c) {
		if (pointInTriangle2D(center, a, b, c))
			return 0.0f;
		return std::min({
			pointSegmentDistanceSquared2D(center, a, b),
			pointSegmentDistanceSquared2D(center, b, c),
			pointSegmentDistanceSquared2D(center, c, a)
		});
	}

	float exhaustApertureRadius(
		const gen_model::spaceship::EngineSettings& engine,
		int nozzleCells
	) {
		const float cellRadius = nozzleCellRadius(engine.radius, nozzleCells);
		float maximumOffset = 0.0f;
		for (const Point3 offset : nozzleCellOffsets(nozzleCells, engine.radius))
			maximumOffset = std::max(
				maximumOffset,
				std::sqrt(offset.x * offset.x + offset.y * offset.y)
			);
		// Keep a small manufacturing clearance around the visible cell envelope;
		// structure may surround the nozzle, but it must not enter the exhaust throat.
		return maximumOffset + cellRadius + engine.radius * 0.04f;
	}

	void requireClearExhaustApertures(
		const gen_model::spaceship::Settings& settings,
		const std::vector<gen_model::spaceship::design::EnginePod>* resolvedPods,
		const TaggedMesh& taggedMesh
	) {
		for (std::size_t engineIndex = 0; engineIndex < settings.engines.size(); ++engineIndex) {
			const auto& engine = settings.engines[engineIndex];
			const int nozzleCells = resolvedPods != nullptr && engineIndex < resolvedPods->size()
				? std::clamp((*resolvedPods)[engineIndex].nozzleCells, 1, 6)
				: 1;
			const float apertureRadius = exhaustApertureRadius(engine, nozzleCells);
			const float rearZ = engine.center.z - engine.length * 0.5f - engine.nozzleDepth;
			// Only the throat immediately behind the aperture is a firing-path
			// keepout.  The nacelle's internal bulkhead may sit farther forward, but
			// no armor, service brace, or collar may intrude into the first 45% of the
			// exhaust opening where it would visibly stick through the hole.
			const float frontZ = rearZ + std::max(
				engine.nozzleDepth * 0.45f,
				engine.radius * 0.08f
			);
			const float minimumZ = rearZ - engine.radius * 0.005f;
			const float maximumRadiusSquared = apertureRadius * apertureRadius;
			for (std::size_t triangleIndex = 0; triangleIndex < taggedMesh.mesh.triangles.size(); ++triangleIndex) {
				const auto& tag = taggedMesh.tags[triangleIndex];
				if (tag.surface == Surface::Engine)
					continue;
				const auto& triangle = taggedMesh.mesh.triangles[triangleIndex];
				const Point3 a = taggedMesh.mesh.positions[triangle.positionIndices[0]];
				const Point3 b = taggedMesh.mesh.positions[triangle.positionIndices[1]];
				const Point3 c = taggedMesh.mesh.positions[triangle.positionIndices[2]];
				const float triangleMinimumZ = std::min({a.z, b.z, c.z});
				const float triangleMaximumZ = std::max({a.z, b.z, c.z});
				if (triangleMaximumZ < minimumZ - EPSILON || triangleMinimumZ > frontZ + EPSILON)
					continue;
				if (triangleDistanceSquared2D(engine.center, a, b, c) > maximumRadiusSquared)
					continue;
				std::ostringstream message;
				message << "Engine aperture obstruction: engine=" << engineIndex
					<< " triangle=" << triangleIndex
					<< " surface=" << static_cast<int>(tag.surface)
					<< " center=(" << engine.center.x << ',' << engine.center.y << ',' << engine.center.z << ')'
					<< " apertureRadius=" << apertureRadius
					<< " zRange=[" << minimumZ << ',' << frontZ << ']'
					<< " vertices=[(" << a.x << ',' << a.y << ',' << a.z << ")"
					<< ",(" << b.x << ',' << b.y << ',' << b.z << ")"
					<< ",(" << c.x << ',' << c.y << ',' << c.z << ")]";
				throw std::invalid_argument(message.str());
			}
		}
	}

	void addThrusterArchitectureDetails(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::EngineSettings& engine,
		int nozzleCells
	) {
		const float rearZ = engine.center.z - engine.length * 0.5f - engine.nozzleDepth;
		const float r = engine.radius;
		const int segments = std::clamp(settings.cylinderSegments, 8, 16);
		const int collarSegments = [&]() {
			switch (settings.design.propulsionLayout) {
				case PropulsionLayout::SpineCluster: return 8;
				case PropulsionLayout::TwinBoom: return 10;
				case PropulsionLayout::WingNacelles: return 16;
				case PropulsionLayout::DistributedAft: return 12;
				case PropulsionLayout::CapitalSideBlocks: return 8;
				case PropulsionLayout::CentralCluster: return 10;
				case PropulsionLayout::Auto:
				default: return segments;
			}
		}();
		float maximumCollarRadius = std::numeric_limits<float>::max();
		for (const auto& other : settings.engines) {
			if (&other == &engine)
				continue;
			const float lateralDistance = std::sqrt(
				(other.center.x - engine.center.x) * (other.center.x - engine.center.x)
				+ (other.center.y - engine.center.y) * (other.center.y - engine.center.y)
			);
			if (lateralDistance > EPSILON)
				maximumCollarRadius = std::min(
					maximumCollarRadius,
					lateralDistance * 0.44f
				);
		}
		const auto collar = [&](float radiusScale, float depthScale, Surface surface, float zOffset) {
			// When paired engine pods are closer than one pod radius, the inner
			// collar has no independent structural room. The outer retention ring
			// already carries the throat; suppressing this nested ring prevents two
			// coplanar annular skins from occupying the same tight engine bay.
			if (surface == Surface::Engine
				&& maximumCollarRadius < r * 0.90f)
				return;
			const float depth = std::max(engine.nozzleDepth * depthScale, r * 0.07f);
			const float centerZ = rearZ - depth * (0.72f + zOffset * 0.08f);
			const float collarRadiusLimit = surface == Surface::Engine
				? maximumCollarRadius * 0.78f
				: maximumCollarRadius;
			const float outerRadius = std::min(
				r * radiusScale,
				collarRadiusLimit
			);
			const float openingRadius = nozzleCellRadius(r, nozzleCells) + r * 0.04f;
			// A collar is a hollow structural ring, never a capped disk.  Its clear
			// inner throat is larger than every nozzle cell envelope, so the exhaust
			// aperture remains visibly open and the topology stays closed by annular
			// front/rear faces rather than a surface across the hole.
			if (outerRadius <= openingRadius + r * 0.02f)
				return;
			const float innerRadius = std::min(
				outerRadius * 0.76f,
				outerRadius - r * 0.04f
			);
			if (innerRadius <= openingRadius || innerRadius >= outerRadius - EPSILON)
				return;
			for (int segment = 0; segment < collarSegments; ++segment) {
				const float angle = 2.0f * PI * static_cast<float>(segment)
					/ static_cast<float>(collarSegments);
				const float nextAngle = 2.0f * PI * static_cast<float>(segment + 1)
					/ static_cast<float>(collarSegments);
				const float cosine = std::cos(angle);
				const float sine = std::sin(angle);
				const float nextCosine = std::cos(nextAngle);
				const float nextSine = std::sin(nextAngle);
				const float rear = centerZ - depth * 0.5f;
				const float front = centerZ + depth * 0.5f;
				const Point3 outerRear{
					engine.center.x + cosine * outerRadius,
					engine.center.y + sine * outerRadius,
					rear
				};
				const Point3 outerFront{
					engine.center.x + cosine * outerRadius,
					engine.center.y + sine * outerRadius,
					front
				};
				const Point3 outerRearNext{
					engine.center.x + nextCosine * outerRadius,
					engine.center.y + nextSine * outerRadius,
					rear
				};
				const Point3 outerFrontNext{
					engine.center.x + nextCosine * outerRadius,
					engine.center.y + nextSine * outerRadius,
					front
				};
				const Point3 innerRear{
					engine.center.x + cosine * innerRadius,
					engine.center.y + sine * innerRadius,
					rear
				};
				const Point3 innerFront{
					engine.center.x + cosine * innerRadius,
					engine.center.y + sine * innerRadius,
					front
				};
				const Point3 innerRearNext{
					engine.center.x + nextCosine * innerRadius,
					engine.center.y + nextSine * innerRadius,
					rear
				};
				const Point3 innerFrontNext{
					engine.center.x + nextCosine * innerRadius,
					engine.center.y + nextSine * innerRadius,
					front
				};
				const Point3 radial{cosine, sine, 0.0f};
				builder.addQuad(
					outerRear, outerFront, outerFrontNext, outerRearNext,
					radial, surface
				);
				builder.addQuad(
					innerRearNext, innerFrontNext, innerFront, innerRear,
					radial * -1.0f, surface
				);
				builder.addQuad(
					outerRearNext, outerRear, innerRear, innerRearNext,
					{0.0f, 0.0f, -1.0f}, surface
				);
				builder.addQuad(
					outerFront, outerFrontNext, innerFrontNext, innerFront,
					{0.0f, 0.0f, 1.0f}, surface
				);
			}
		};

		// The nozzle cells are the thrust hardware; the architecture-specific
		// collars and shrouds below make the rear read as a maintained machine
		// instead of the same cylinder repeated on every hull grammar.
		switch (settings.design.propulsionLayout) {
			case PropulsionLayout::CentralCluster:
				collar(1.08f, 0.32f, Surface::Structure, 0.18f);
				collar(0.86f, 0.16f, Surface::Engine, 0.56f);
				if (std::abs(engine.center.x) <= EPSILON) {
					builder.addTaperedBeam(
						{engine.center.x, engine.center.y + r * 0.92f, rearZ + r * 0.18f},
						{engine.center.x, engine.center.y + r * 1.28f, rearZ + r * 0.18f},
						r * 0.22f,
						r * 0.12f,
						r * 0.18f,
						Surface::Structure
					);
				}
				break;
			case PropulsionLayout::SpineCluster:
				collar(1.16f, 0.22f, Surface::Structure, 0.12f);
				collar(0.76f, 0.26f, Surface::Engine, 0.52f);
				builder.addBox(
					{engine.center.x, engine.center.y + r * 0.96f, rearZ + r * 0.05f},
					{r * 0.34f, r * 0.22f, r * 0.42f},
					Surface::Structure
				);
				break;
			case PropulsionLayout::TwinBoom:
				collar(1.18f, 0.18f, Surface::Structure, 0.10f);
				collar(0.72f, 0.30f, Surface::Engine, 0.52f);
				for (const float side : {-1.0f, 1.0f}) {
					builder.addBox(
						{engine.center.x + side * r * 0.92f, engine.center.y, rearZ + r * 0.18f},
						{r * 0.20f, r * 1.42f, r * 0.34f},
						Surface::Structure
					);
				}
				break;
			case PropulsionLayout::WingNacelles:
				collar(1.24f, 0.24f, Surface::Structure, 0.12f);
				collar(0.74f, 0.32f, Surface::Engine, 0.55f);
				for (const float side : {-1.0f, 1.0f}) {
					// The wing-side thrust brace begins ahead of the nozzle throat.
					// Starting it at the rear station made its diagonal section dip
					// through the opening even though the socket itself was lateral.
					const float rootZ = rearZ + r * 0.75f;
					addTaperedPlanFairing(
						builder,
						{engine.center.x + side * r * 0.62f, engine.center.y + r * 0.18f, rootZ},
						{engine.center.x + side * r * 1.55f, engine.center.y + r * 0.10f, rootZ + r * 0.46f},
						r * 0.44f,
						r * 0.22f,
						engine.center.y + r * 0.54f,
						engine.center.y - r * 0.30f,
						engine.center.y + r * 0.30f,
						engine.center.y - r * 0.16f,
						Surface::Structure
					);
				}
				break;
			case PropulsionLayout::DistributedAft:
				collar(1.28f, 0.26f, Surface::Structure, 0.10f);
				collar(0.68f, 0.34f, Surface::Engine, 0.56f);
				builder.addBox(
					// This ventral service pod must clear the nozzle circle, not just
					// the nozzle centre.  Its upper edge was close enough to bridge
					// the lower half of the aperture on the dense terminator bank.
					{engine.center.x, engine.center.y - r * 1.15f, rearZ + r * 0.12f},
					{r * 1.18f, r * 0.24f, r * 0.28f},
					Surface::Structure
				);
				break;
			case PropulsionLayout::CapitalSideBlocks:
				collar(1.36f, 0.30f, Surface::Structure, 0.08f);
				collar(0.78f, 0.38f, Surface::Engine, 0.58f);
				builder.addBox(
					// Keep the service block above the full nozzle-cell envelope.  The
					// capital side pod is broad enough that a box centered only 0.78r
					// above the throat would still intrude into the exhaust aperture.
					{engine.center.x, engine.center.y + r * 1.35f, rearZ + r * 0.10f},
					{r * 1.65f, r * 0.30f, r * 0.52f},
					Surface::Structure
				);
				break;
			case PropulsionLayout::Auto:
			default:
				collar(1.12f, 0.22f, Surface::Structure, 0.12f);
				collar(0.78f, 0.30f, Surface::Engine, 0.54f);
				break;
		}

		// A multi-cell pod gets a thin rear retention ring.  This is intentionally
		// smaller than the shroud so every nozzle aperture remains visible.
		if (nozzleCells > 1
			&& maximumCollarRadius >= r * 0.90f)
			collar(0.92f, 0.10f, Surface::Structure, 0.84f);
	}

	void addAftMachineryEnvelope(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings
	) {
		const float rear = -settings.hull.length * 0.48f;
		const float height = settings.hull.height;
		const bool fighter = isFighterArchetype(settings);
		// Fighters already have a closed pressure hull, wing roots, and visible
		// nacelles carrying the aft load path.  Any additional generic tail shell
		// becomes a detached dorsal "butt" behind the cockpit, so leave the rear
		// silhouette to those authored components.
		if (fighter)
			return;
		float tailCenterY = -height * 0.10f;
		// The pressure hull itself is already a closed aft volume.  Do not emit a
		// second generic tail shell here: it sat behind the authored hull and read as
		// a displaced rectangular butt in rear/side views.  Architecture-specific
		// load paths below remain available for ships that actually need them.
		switch (settings.design.propulsionLayout) {
			case PropulsionLayout::CentralCluster:
				addTaperedPlanFairing(
					builder,
					{0.0f, -height * 0.08f, rear + settings.hull.length * 0.12f},
					{0.0f, -height * 0.04f, rear - settings.hull.length * 0.10f},
					settings.hull.width * 0.42f,
					settings.hull.width * 0.26f,
					height * 0.54f,
					height * 0.34f,
					height * 0.46f,
					height * 0.28f,
					Surface::Structure
				);
				break;
			case PropulsionLayout::SpineCluster:
				builder.addTaperedBeam(
					{0.0f, -height * 0.34f, rear + settings.hull.length * 0.18f},
					{0.0f, -height * 0.42f, rear - settings.hull.length * 0.16f},
					settings.hull.width * 0.18f,
					settings.hull.width * 0.10f,
					height * 0.22f,
					Surface::Structure
				);
				break;
		case PropulsionLayout::TwinBoom:
		case PropulsionLayout::WingNacelles:
			{
				const float aftBeamCenterY = isFighterArchetype(settings)
					? tailCenterY
					: -height * 0.08f;
				for (const float side : {-1.0f, 1.0f})
					builder.addTaperedBeam(
						{side * settings.wings.halfSpan * 0.38f, aftBeamCenterY, rear + settings.hull.length * 0.14f},
						{side * settings.wings.halfSpan * 0.48f, aftBeamCenterY, rear - settings.hull.length * 0.16f},
						settings.wings.shoulderWidth * 0.72f,
						settings.wings.shoulderWidth * 0.42f,
						height * 0.24f,
						Surface::Structure
					);
			}
			break;
			case PropulsionLayout::DistributedAft:
				builder.addBox(
					{0.0f, -height * 0.10f, rear},
					{settings.hull.width * 0.72f, height * 0.52f, height * 0.18f},
					Surface::Structure
				);
				break;
			case PropulsionLayout::CapitalSideBlocks:
				for (const float side : {-1.0f, 1.0f})
				{
					// Side-block engines occupy the inner capital shoulders.  Put the
					// aft service rails on the outer wing edge instead of spanning across
					// a nozzle throat between the engine banks.
					const float sideRailX = side * settings.wings.halfSpan * 0.88f;
					addTaperedPlanFairing(
						builder,
						{sideRailX, -height * 0.06f, rear + settings.hull.length * 0.16f},
						{sideRailX, -height * 0.04f, rear - settings.hull.length * 0.14f},
						settings.hull.width * 0.16f,
						settings.hull.width * 0.11f,
						height * 0.34f,
						-height * 0.34f,
						height * 0.24f,
						-height * 0.24f,
						Surface::Structure
					);
				}
				break;
			case PropulsionLayout::Auto:
			default:
				break;
		}
	}

	void addEngines(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const std::vector<gen_model::spaceship::design::EnginePod>* resolvedPods = nullptr
	) {
		for (std::size_t engineIndex = 0; engineIndex < settings.engines.size(); ++engineIndex) {
			const auto& engine = settings.engines[engineIndex];
			const bool fighter = isFighterArchetype(settings);
			const bool distributedAft = settings.design.propulsionLayout == PropulsionLayout::DistributedAft;
			const Point3 fairingRoot{
				distributedAft ? engine.center.x : engine.center.x * 0.18f,
				distributedAft
					? engine.center.y + engine.radius * 0.30f
					: engine.center.y + engine.radius * 0.22f,
				distributedAft
					? engine.center.z + engine.length * 0.30f
					: engine.center.z + engine.length * 0.36f
			};
			const Point3 fairingTip{
				engine.center.x,
				engine.center.y + engine.radius * 0.08f,
				// Stop the diagonal service fairing well ahead of the nozzle throat.
				// Its plan-width projects into Z, so ending at the nacelle's rear
				// station would otherwise put armor across the exhaust opening.
				engine.center.z - engine.length * 0.18f
			};
			if (!fighter) {
				addTaperedPlanFairing(
					builder,
					fairingRoot,
					fairingTip,
					distributedAft ? engine.radius * 1.90f : engine.radius * 4.6f,
					distributedAft ? engine.radius * 1.30f : engine.radius * 1.85f,
					distributedAft ? engine.center.y + engine.radius * 0.54f : settings.wings.topY + 0.14f,
					distributedAft ? engine.center.y - engine.radius * 0.54f : settings.wings.bottomY - 0.05f,
					distributedAft ? engine.center.y + engine.radius * 0.34f : engine.center.y + engine.radius * 0.30f,
					distributedAft ? engine.center.y - engine.radius * 0.34f : engine.center.y - engine.radius * 0.50f,
					Surface::Structure
				);
			}
			// On fighters the nacelle is a shallow thrust-cell inside the lifting
			// body; only the nozzle is exposed.  Capital ships retain a dark service
			// housing because their engines are accessible from the deck.
			addFacetedNacelle(builder, engine, settings.cylinderSegments, fighter ? Surface::Armor : Surface::Structure);
			const Point3 nozzleCenter{
				engine.center.x,
				engine.center.y,
				engine.center.z - engine.length * 0.5f - engine.nozzleDepth * 0.5f
			};
			const int nozzleCells = resolvedPods != nullptr && engineIndex < resolvedPods->size()
				? std::clamp((*resolvedPods)[engineIndex].nozzleCells, 1, 6)
				: 1;
			const float cellRadius = nozzleCellRadius(engine.radius, nozzleCells);
			for (const Point3 offset : nozzleCellOffsets(nozzleCells, engine.radius))
				builder.addCylinderZ(
					nozzleCenter + offset,
					cellRadius,
					engine.nozzleDepth,
					settings.cylinderSegments,
					Surface::Engine
				);
			addThrusterArchitectureDetails(builder, settings, engine, nozzleCells);
		}
		addAftMachineryEnvelope(builder, settings);
	}

	void addArmorLandmarks(MeshBuilder& builder, const gen_model::spaceship::Settings& settings) {
		// Fighter skins carry access hatches in the panel atlas; extra external
		// bars would read as loose accessories.  The larger hulls keep a visible
		// service spine because those crews actually walk the deck.
		if (settings.layout.archetype != "siege_gunship" && settings.layout.archetype != "carrier")
			return;
		const float hullTop = settings.hull.height * 0.52f + settings.hull.crown;
		const float panelY = hullTop + settings.armorDepth * 0.34f;
		addTaperedPlanFairing(
			builder,
			{0.0f, panelY, -settings.hull.length * 0.44f},
			{0.0f, panelY + settings.armorDepth * 0.06f, settings.hull.length * 0.04f},
			settings.hull.width * 0.46f,
			settings.hull.width * 0.30f,
			panelY + settings.armorDepth * 0.24f,
			panelY - settings.armorDepth * 0.20f,
			panelY + settings.armorDepth * 0.18f,
			panelY - settings.armorDepth * 0.16f,
			Surface::Armor
		);
		for (const float side : {1.0f, -1.0f}) {
			addTaperedPlanFairing(
				builder,
				// Keep the service fin just inside the rear pressure-skin station.
				// Starting it on the hull's rear cap creates a coplanar triangular
				// patch at the exact aft plane.
				{side * settings.hull.width * 0.45f, 0.0f, -settings.hull.length * 0.46f},
				{side * settings.hull.width * 0.45f, settings.hull.height * 0.10f, -settings.hull.length * 0.03f},
				settings.armorDepth * 1.7f,
				settings.armorDepth * 0.95f,
				settings.hull.height * 0.22f,
				-settings.hull.height * 0.24f,
				settings.hull.height * 0.28f,
				-settings.hull.height * 0.18f,
				Surface::Structure
			);
		}
	}

	void addSystemsLayout(MeshBuilder& builder, const gen_model::spaceship::Settings& settings) {
		if (isFighterArchetype(settings)) {
			// The fighter reactor, fuel cells, and longeron are pressure-contained.
			// Exposing the helper nacelle through the low shell made an otherwise
			// coherent airframe look like a dark box bolted underneath it.  The
			// engines and flush service seams already communicate the thrust path;
			// keep the actual power plant inside the generated pressure volume.
			return;
		}
		// The primary longeron carries engine thrust through the reactor/fuel mass and
		// into the nose.  Most of it is buried, but its ventral edge remains readable.
		builder.addTaperedBeam(
			{0.0f, -settings.hull.height * 0.42f, -settings.hull.length * 0.49f},
			{0.0f, -settings.hull.height * 0.30f, settings.hull.length * 0.40f},
			settings.layout.primarySpineWidth * 1.18f,
			settings.layout.primarySpineWidth * 0.72f,
			settings.layout.primarySpineWidth * 0.40f,
			Surface::Structure
		);

		if (settings.layout.archetype == "siege_gunship") {
			for (const float side : {1.0f, -1.0f}) {
				const gen_model::spaceship::EngineSettings weaponSponson{
					{side * settings.wings.halfSpan * 0.72f, 0.0f, -settings.hull.length * 0.05f},
					settings.hull.height * 0.28f,
					settings.hull.length * 0.86f,
					settings.armorDepth
				};
				addFacetedNacelle(builder, weaponSponson, settings.cylinderSegments, Surface::Armor);
				for (const float deck : {1.0f, -1.0f}) {
					const float deckY = deck * settings.hull.height * 0.43f;
					addTaperedPlanFairing(
						builder,
						{side * settings.wings.halfSpan * 0.78f, deckY, -settings.hull.length * 0.40f},
						{side * settings.wings.halfSpan * 0.72f, deckY, settings.hull.length * 0.30f},
						settings.hull.height * 0.62f,
						settings.hull.height * 0.44f,
						deckY + settings.armorDepth * 0.78f,
						deckY - settings.armorDepth * 0.78f,
						deckY + settings.armorDepth * 0.58f,
						deckY - settings.armorDepth * 0.58f,
						Surface::Structure
					);
				}
			}
		}

		// The carrier's broad wing/deck is already the flight surface.  Keep the
		// pressure-door and service-bay cues in the structure/normal atlas rather than
		// adding a second long pod that would read as a bolted-on barge.
	}

	[[maybe_unused]] void addCurvedLoadFairing(
		MeshBuilder& builder,
		Point3 root,
		Point3 socket,
		const std::array<float, 4>& widths,
		const std::array<float, 4>& tops,
		const std::array<float, 4>& bottoms,
		float crown,
		Surface surface,
		int mountOwner
	) {
		const Point3 delta{socket.x - root.x, 0.0f, socket.z - root.z};
		const float deltaLength = length(delta);
		if (deltaLength <= EPSILON)
			return;
		const Point3 axis = delta * (1.0f / deltaLength);
		const float sideSign = std::abs(root.x + socket.x) > EPSILON
			? (root.x + socket.x >= 0.0f ? 1.0f : -1.0f)
			: 1.0f;
		const Point3 lateral{-axis.z * sideSign, 0.0f, axis.x * sideSign};
		const std::array<float, 4> amounts{0.0f, 0.34f, 0.70f, 1.0f};
		std::vector<std::vector<Point3>> rings(amounts.size());
		for (std::size_t station = 0; station < amounts.size(); ++station) {
			const float amount = amounts[station];
			const Point3 center = root + (socket - root) * amount;
			// `tops`/`bottoms` are absolute ship-space skin heights.  Keep the
			// longitudinal centerline from root/socket, but do not add its Y a
			// second time or the fairing will float above the wing at the socket.
			const Point3 centerXZ{center.x, 0.0f, center.z};
			const float halfWidth = widths[station] * 0.5f;
			const float localCrown = crown * (1.0f - amount * 0.45f);
			const float top = tops[station];
			const float bottom = bottoms[station];
			// A rounded section spreads recoil into the wing at the root and avoids
			// the fan-triangle wedges that previously read as teeth in QA renders.
			rings[station] = {
				centerXZ - lateral * halfWidth + Point3{0.0f, top, 0.0f},
				centerXZ - lateral * (halfWidth * 0.52f) + Point3{0.0f, top + localCrown * 0.66f, 0.0f},
				centerXZ + Point3{0.0f, top + localCrown, 0.0f},
				centerXZ + lateral * (halfWidth * 0.52f) + Point3{0.0f, top + localCrown * 0.66f, 0.0f},
				centerXZ + lateral * halfWidth + Point3{0.0f, top, 0.0f},
				centerXZ + lateral * halfWidth + Point3{0.0f, bottom, 0.0f},
				centerXZ + lateral * (halfWidth * 0.52f) + Point3{0.0f, bottom - localCrown * 0.24f, 0.0f},
				centerXZ + Point3{0.0f, bottom - localCrown * 0.34f, 0.0f},
				centerXZ - lateral * (halfWidth * 0.52f) + Point3{0.0f, bottom - localCrown * 0.24f, 0.0f},
				centerXZ - lateral * halfWidth + Point3{0.0f, bottom, 0.0f}
			};
		}
		builder.addClosedLoft(
			rings,
			surface,
			surface,
			surface,
			mountOwner
		);
	}

	[[maybe_unused]] void addFlushMountFairing(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::MountSettings& mount,
		Point3 socketCenter
	) {
		const Point3 delta{socketCenter.x - mount.supportRoot.x, 0.0f, socketCenter.z - mount.supportRoot.z};
		if (lengthSquared(delta) <= EPSILON)
			return;
		const float baseWidth = std::max(
			mount.supportWidth * 1.15f,
			mount.turretRadius * 2.00f
		);
		const float middleWidth = std::max(
			mount.supportWidth * 0.90f,
			mount.turretRadius * 1.65f
		);
		// Keep the countersunk neck narrower than the turret sphere so its leading
		// edge cannot intrude into a forward barrel as the weapon slews.
		const float socketWidth = mount.turretRadius * 1.05f;
		// Keep the fairing inside the wing's airfoil envelope.  The support root is
		// a load-path coordinate, not a license to raise a separate pylon above the
		// skin; the hull/wing intersection supplies the actual structural depth.
		const float baseTop = wingRootTop(settings);
		// The turret sphere is the visible weapon body.  Seat the cylinder below the
		// wing and bring the armor fairing up to the sphere's lower tangent, so the
		// holder is a countersunk part of the airfoil rather than an exposed puck.
		const float socketTop = mount.position.y - mount.turretRadius * 0.96f;
		// This is a countersunk wing doubler, not a freestanding pylon.  A shallow
		// lower skin keeps the load spreader structurally wide while preserving the
		// airframe's clean side silhouette and the turret's forward corridor.
		const float skinDepth = std::max(settings.armorDepth * 0.55f, 0.045f);
		const float baseSkinBottom = baseTop - skinDepth;
		const float socketBottom = socketTop - skinDepth;
		const float middleTop = (baseTop + socketTop) * 0.52f;
		const float middleBottom = (baseSkinBottom + socketBottom) * 0.52f;
		addCurvedLoadFairing(
			builder,
			mount.supportRoot,
			socketCenter,
			{baseWidth, middleWidth, middleWidth * 0.72f, socketWidth},
			{baseTop, middleTop, (middleTop + socketTop) * 0.58f, socketTop},
			{baseSkinBottom, middleBottom, (middleBottom + socketBottom) * 0.58f, socketBottom},
			std::max(settings.armorDepth * 0.26f, mount.supportHeight * 0.10f),
			Surface::Armor,
			-1
		);
	}

	[[maybe_unused]] void addDeckMountFairing(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::MountSettings& mount,
		Point3 supportStart,
		Point3 socketCenter
	) {
		const Point3 delta{socketCenter.x - supportStart.x, 0.0f, socketCenter.z - supportStart.z};
		if (lengthSquared(delta) <= EPSILON)
			return;
		const bool carrier = settings.layout.archetype == "carrier";
		// Carrier deck sockets sit on a broad flight surface, but their load path
		// must still leave a clean forward barrel corridor.  Keep the doubler wide
		// enough to spread recoil without letting its leading edge rise into the
		// turret's forward envelope.
		const float baseWidth = mount.supportWidth * (carrier ? 1.4f : 3.0f);
		const float socketWidth = mount.turretRadius * 2.2f;
		const float baseDepth = mount.supportHeight * (carrier ? 0.32f : 0.52f);
		const float baseTop = supportStart.y + baseDepth;
		const float baseBottom = supportStart.y - baseDepth;
		const float deckSide = mount.position.y < 0.0f ? -1.0f : 1.0f;
		// The holder grows toward the deck side of the turret.  Mirroring the
		// vertical offsets keeps the fairing out of the forward barrel corridor on
		// both dorsal and ventral sockets; the bearing cylinder remains the intentional
		// pivot and is excluded from the obstruction audit.
		const float socketTop = socketCenter.y - deckSide * mount.socketHeight * 0.28f;
		const float socketBottom = socketCenter.y - deckSide * mount.socketHeight * 0.54f;
		// The deck is the load-spreading armor of the airframe.  Use the same rounded
		// longitudinal fairing as the other service structures so a capital socket
		// reads as a grown saddle, not a thin triangular plate bolted to the deck.
		addTaperedPlanFairing(
			builder,
			supportStart,
			socketCenter,
			baseWidth,
			socketWidth,
			baseTop,
			baseBottom,
			socketTop,
			socketBottom,
			Surface::Armor
		);
	}

	Point3 hermitePoint(
		Point3 start,
		Point3 startTangent,
		Point3 end,
		Point3 endTangent,
		float amount
	) {
		const float amountSquared = amount * amount;
		const float amountCubed = amountSquared * amount;
		return start * (2.0f * amountCubed - 3.0f * amountSquared + 1.0f)
			+ startTangent * (amountCubed - 2.0f * amountSquared + amount)
			+ end * (-2.0f * amountCubed + 3.0f * amountSquared)
			+ endTangent * (amountCubed - amountSquared);
	}

	Point3 hermiteDirection(
		Point3 start,
		Point3 startTangent,
		Point3 end,
		Point3 endTangent,
		float amount
	) {
		const float amountSquared = amount * amount;
		return start * (6.0f * amountSquared - 6.0f * amount)
			+ startTangent * (3.0f * amountSquared - 4.0f * amount + 1.0f)
			+ end * (-6.0f * amountSquared + 6.0f * amount)
			+ endTangent * (3.0f * amountSquared - 2.0f * amount);
	}

	void addSweptLoadPathFairing(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::MountSettings& mount,
		const SurfaceAttachment& attachment,
		int mountOwner,
		bool mirrorX
	) {
		const auto place = [mirrorX](Point3 point) {
			if (mirrorX)
				point.x = -point.x;
			return point;
		};
		const float embed = settings.armorDepth * 0.08f;
		const Point3 start = attachment.surfacePoint - attachment.surfaceNormal * embed;
		const Point3 end = attachment.blisterBase;
		const float curveLength = length(end - start);
		if (curveLength <= EPSILON)
			return;
		// A long exposed cable-like curve reads as a tentacle in silhouette.  Use a
		// straight, broad load saddle instead: the optimizer still chooses the
		// structurally useful surface point, while the fairing follows the shortest
		// load path and tapers into the bearing like an armored wing doubler.
		const Point3 path = end - start;
		const Point3 startTangent = path;
		const Point3 endTangent = path;
		const float startWidth = std::max(
			mount.supportWidth * 3.0f, mount.turretRadius * 4.2f
		);
		const float endWidth = std::max(
			mount.supportWidth * 0.95f, mount.turretRadius * 1.70f
		);
		const float startHeight = std::max(
			settings.armorDepth * 0.48f, mount.supportHeight * 0.30f
		);
		const float endHeight = std::max(
			settings.armorDepth * 0.36f, mount.supportHeight * 0.22f
		);
		const std::array<float, 6> amounts{0.0f, 0.18f, 0.38f, 0.62f, 0.82f, 1.0f};
		const int ringSegments = std::clamp(settings.cylinderSegments, 10, 16);
		std::vector<std::vector<Point3>> rings;
		rings.reserve(amounts.size());
		for (const float amount : amounts) {
			const Point3 center = hermitePoint(
				start, startTangent, end, endTangent, amount
			);
			Point3 axis = hermiteDirection(
				start, startTangent, end, endTangent, amount
			);
			if (lengthSquared(axis) <= EPSILON)
				axis = end - start;
			axis = gen_model::gen_types::normalize(axis);
			Point3 reference = attachment.surfaceNormal;
			if (std::abs(gen_model::gen_types::dot(axis, reference)) > 0.95f)
				reference = std::abs(axis.y) < 0.90f
					? Point3{0.0f, 1.0f, 0.0f}
					: Point3{0.0f, 0.0f, 1.0f};
			Point3 right = gen_model::gen_types::normalize(
				gen_model::gen_types::cross(reference, axis)
			);
			Point3 up = gen_model::gen_types::normalize(
				gen_model::gen_types::cross(axis, right)
			);
			if (gen_model::gen_types::dot(up, attachment.surfaceNormal) < 0.0f) {
				right = right * -1.0f;
				up = up * -1.0f;
			}
			const float taper = smoothStep(amount);
			const float halfWidth = (
				startWidth + (endWidth - startWidth) * taper
			) * 0.5f;
			const float halfHeight = (
				startHeight + (endHeight - startHeight) * taper
			) * 0.5f;
			std::vector<Point3> ring;
			ring.reserve(static_cast<std::size_t>(ringSegments));
			for (int segment = 0; segment < ringSegments; ++segment) {
				const float angle = 2.0f * PI * static_cast<float>(segment)
					/ static_cast<float>(ringSegments);
				ring.push_back(symmetrySnapPoint(
					center + right * (std::cos(angle) * halfWidth)
					+ up * (std::sin(angle) * halfHeight)
				));
			}
			rings.push_back(std::move(ring));
		}
		for (auto& ring : rings) {
			for (Point3& point : ring)
				point = place(point);
		}
		builder.addClosedLoft(
			rings,
			Surface::Armor,
			Surface::Armor,
			Surface::Armor,
			mountOwner
		);
	}

	void addTangentBlister(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::MountSettings& mount,
		const SurfaceAttachment& attachment,
		int mountOwner,
		bool mirrorX,
		float footprintLimit
	) {
		const auto place = [mirrorX](Point3 point) {
			if (mirrorX)
				point.x = -point.x;
			return point;
		};
		const Point3 normal = attachment.blisterNormal;
		Point3 major = attachment.surfaceTangent
			- normal * gen_model::gen_types::dot(attachment.surfaceTangent, normal);
		if (lengthSquared(major) <= EPSILON) {
			const Point3 reference = std::abs(normal.y) < 0.9f
				? Point3{0.0f, 1.0f, 0.0f}
				: Point3{0.0f, 0.0f, 1.0f};
			major = gen_model::gen_types::cross(reference, normal);
		}
		major = gen_model::gen_types::normalize(major);
		Point3 minor = gen_model::gen_types::normalize(
			gen_model::gen_types::cross(normal, major)
		);
		major = gen_model::gen_types::normalize(
			gen_model::gen_types::cross(minor, normal)
		);
		const float unconstrainedMajorRadius = std::max(
			mount.supportWidth * 0.68f,
			mount.turretRadius * settings.mountAttachment.blisterRadiusScale
		);
		const float unconstrainedMinorRadius = std::max(
			mount.supportWidth * 0.56f,
			mount.turretRadius * settings.mountAttachment.blisterRadiusScale * 0.78f
		);
		// A dense battery is still made from individual bearing blisters, not a
		// single intersecting slab.  Limit each footprint to the nearest resolved
		// socket spacing while retaining the authored support/turret proportions.
		// This is resolved before export, so the mount report and the visible socket
		// stay at the same coordinates and no post-build translation is required.
		const float majorRadius = std::min(unconstrainedMajorRadius, footprintLimit);
		const float minorRadius = std::min(
			unconstrainedMinorRadius,
			footprintLimit * 0.84f
		);
		const float embed = settings.armorDepth * 0.06f;
		const Point3 snappedBase = symmetrySnapPoint(attachment.blisterBase);
		const Point3 apex = symmetrySnapPoint(attachment.lowerTangent);
		const Point3 apexDelta = apex - snappedBase;
		const float normalHeight = std::max(
			gen_model::gen_types::dot(apexDelta, normal), settings.armorDepth * 0.25f
		);
		const Point3 tangentDelta = apexDelta - normal * normalHeight;
		const Point3 boundaryCenter = snappedBase - normal * embed;
		const std::array<float, 4> normalizedRadii{1.0f, 0.78f, 0.52f, 0.27f};
		const int segments = std::clamp(settings.cylinderSegments, 12, 20);
		std::vector<std::vector<Point3>> rings;
		rings.reserve(normalizedRadii.size());
		for (const float radius : normalizedRadii) {
			const float profile = std::pow(1.0f - radius * radius, 3.0f);
			const Point3 center = boundaryCenter + tangentDelta * profile
				+ normal * ((normalHeight + embed) * profile);
			std::vector<Point3> ring;
			ring.reserve(static_cast<std::size_t>(segments));
			for (int segment = 0; segment < segments; ++segment) {
				const float angle = 2.0f * PI * static_cast<float>(segment)
					/ static_cast<float>(segments);
				ring.push_back(symmetrySnapPoint(
					center + major * (std::cos(angle) * majorRadius * radius)
					+ minor * (std::sin(angle) * minorRadius * radius)
				));
			}
			rings.push_back(std::move(ring));
		}
		for (auto& ring : rings) {
			for (Point3& point : ring)
				point = place(point);
		}
		const Point3 placedApex = place(apex);
		const Point3 placedBoundaryCenter = place(boundaryCenter);
		builder.addLoftSides(rings, Surface::Armor, mountOwner, true);
		Point3 baseCenter{};
		Point3 nextCenter{};
		for (const Point3 point : rings.front())
			baseCenter = baseCenter + point;
		for (const Point3 point : rings[1])
			nextCenter = nextCenter + point;
		baseCenter = baseCenter * (1.0f / static_cast<float>(rings.front().size()));
		nextCenter = nextCenter * (1.0f / static_cast<float>(rings[1].size()));
		builder.addConvexCap(
			rings.front(),
			baseCenter - nextCenter,
			Surface::Armor,
			mountOwner,
			true
		);
		for (int segment = 0; segment < segments; ++segment) {
			const int next = (segment + 1) % segments;
			builder.addTriangle(
				rings.back()[segment], placedApex,
				rings.back()[next], placedApex - placedBoundaryCenter,
				Surface::Armor, mountOwner, true
			);
		}
	}

	std::vector<SurfaceAttachment> addMounts(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings
	) {
		std::vector<SurfaceAttachment> attachments;
		attachments.reserve(settings.mounts.size());
		for (const auto& mount : settings.mounts)
			attachments.push_back(chooseAttachment(settings, mount));
		for (std::size_t index = 0; index < settings.mounts.size(); ++index) {
			const auto& mount = settings.mounts[index];
			const SurfaceAttachment& attachment = attachments[index];
			const bool mirrorX = mount.position.x < -EPSILON;
			auto canonicalPoint = [mirrorX](Point3 point) {
				if (mirrorX)
					point.x = -point.x;
				return point;
			};
			auto canonicalMount = mount;
			canonicalMount.position = canonicalPoint(canonicalMount.position);
			canonicalMount.forward = canonicalPoint(canonicalMount.forward);
			canonicalMount.supportRoot = canonicalPoint(canonicalMount.supportRoot);
			auto canonicalAttachment = attachment;
			canonicalAttachment.surfacePoint = canonicalPoint(canonicalAttachment.surfacePoint);
			canonicalAttachment.surfaceNormal = canonicalPoint(canonicalAttachment.surfaceNormal);
			canonicalAttachment.surfaceTangent = canonicalPoint(canonicalAttachment.surfaceTangent);
			canonicalAttachment.lowerTangent = canonicalPoint(canonicalAttachment.lowerTangent);
			canonicalAttachment.blisterBase = canonicalPoint(canonicalAttachment.blisterBase);
			canonicalAttachment.blisterNormal = canonicalPoint(canonicalAttachment.blisterNormal);
			if (!canonicalAttachment.directBlister)
				addSweptLoadPathFairing(
					builder, settings, canonicalMount, canonicalAttachment,
					static_cast<int>(index), mirrorX
				);
			float footprintLimit = std::numeric_limits<float>::max();
			for (std::size_t otherIndex = 0; otherIndex < attachments.size(); ++otherIndex) {
				if (otherIndex == index)
					continue;
				const float distance = length(
					attachments[index].blisterBase - attachments[otherIndex].blisterBase
				);
				if (distance > EPSILON)
					footprintLimit = std::min(footprintLimit, distance * 0.46f);
			}
			addTangentBlister(
				builder, settings, canonicalMount, canonicalAttachment,
				static_cast<int>(index), mirrorX, footprintLimit
			);
		}
		return attachments;
	}

	std::uint32_t hashPixel(int x, int y, std::uint32_t seed) {
		std::uint32_t value = seed;
		value ^= static_cast<std::uint32_t>(x) * 0x9E3779B1u;
		value ^= static_cast<std::uint32_t>(y) * 0x85EBCA77u;
		value ^= value >> 16;
		value *= 0x7FEB352Du;
		value ^= value >> 15;
		value *= 0x846CA68Bu;
		return value ^ (value >> 16);
	}

	float hashUnit(int x, int y, std::uint32_t seed) {
		return static_cast<float>(hashPixel(x, y, seed)) / 4294967295.0f;
	}

	float valueNoise(float x, float y, std::uint32_t seed) {
		const int x0 = static_cast<int>(std::floor(x));
		const int y0 = static_cast<int>(std::floor(y));
		const float xAmount = smoothStep(x - static_cast<float>(x0));
		const float yAmount = smoothStep(y - static_cast<float>(y0));
		auto sample = [&](int sampleX, int sampleY) {
			return hashUnit(sampleX, sampleY, seed);
		};
		const float lower = sample(x0, y0) + (sample(x0 + 1, y0) - sample(x0, y0)) * xAmount;
		const float upper = sample(x0, y0 + 1) + (sample(x0 + 1, y0 + 1) - sample(x0, y0 + 1)) * xAmount;
		return lower + (upper - lower) * yAmount;
	}

	int atlasRegion(float u, float v) {
		return (u >= 0.5f ? 1 : 0) + (v >= 0.5f ? 2 : 0);
	}

	float localAtlasCoordinate(float value) {
		return std::fmod(value * 2.0f, 1.0f);
	}

	struct TextureFeatures {
		int region;
		float localU;
		float localV;
		float secondaryPaint;
		float accentPaint;
		float edgeMask;
		float panelSeam;
		float macro;
		float micro;
		float brushed;
		float vent;
		float thermalDetail;
		float rust;
	};

	struct PaintMasks {
		float secondary = 0.0f;
		float accent = 0.0f;
	};

	float softBand(float distance, float halfWidth, float feather) {
		return 1.0f - smoothRange(halfWidth, halfWidth + feather, std::abs(distance));
	}

	PaintMasks paintMasks(
		float localU,
		float localV,
		const gen_model::spaceship::MaterialSettings& material
	) {
		const float x = std::abs(localU - 0.5f);
		const float secondaryWidth = std::clamp(material.secondaryCoverage, 0.05f, 0.75f);
		const float accentWidth = std::clamp(material.accentCoverage, 0.01f, 0.30f);
		PaintMasks result;
		switch (material.pattern) {
			case gen_model::spaceship::PaintPattern::SpineBand: {
				const float halfWidth = secondaryWidth * 0.50f;
				result.secondary = softBand(x, halfWidth, 0.025f);
				result.accent = gaussianBand(x - halfWidth, 0.010f + accentWidth * 0.09f)
					* smoothRange(0.10f, 0.22f, localV)
					* (1.0f - smoothRange(0.82f, 0.94f, localV));
				break;
			}
			case gen_model::spaceship::PaintPattern::Chevron: {
				const float center = 0.30f + x * 0.72f;
				const float halfWidth = 0.035f + secondaryWidth * 0.16f;
				result.secondary = softBand(localV - center, halfWidth, 0.025f);
				result.accent = gaussianBand(
					localV - center + halfWidth * 0.82f,
					0.009f + accentWidth * 0.10f
				) * smoothRange(0.06f, 0.18f, x);
				break;
			}
			case gen_model::spaceship::PaintPattern::WingBands: {
				const float forwardCenter = 0.66f - x * 0.24f;
				const float aftCenter = 0.31f + x * 0.16f;
				const float halfWidth = 0.020f + secondaryWidth * 0.10f;
				result.secondary = std::max(
					softBand(localV - forwardCenter, halfWidth, 0.020f),
					softBand(localV - aftCenter, halfWidth * 0.72f, 0.018f)
				);
				result.accent = gaussianBand(
					localV - forwardCenter - halfWidth * 0.72f,
					0.008f + accentWidth * 0.09f
				) * smoothRange(0.08f, 0.20f, x);
				break;
			}
			case gen_model::spaceship::PaintPattern::Blocked: {
				const float rearBlock = 1.0f - smoothRange(
					0.34f + secondaryWidth * 0.12f,
					0.41f + secondaryWidth * 0.12f,
					localV
				);
				const float sideBlock = smoothRange(
					0.28f - secondaryWidth * 0.10f,
					0.35f - secondaryWidth * 0.06f,
					x
				) * (1.0f - smoothRange(0.76f, 0.88f, localV));
				result.secondary = std::max(rearBlock, sideBlock);
				result.accent = gaussianBand(localV - 0.43f, 0.018f + accentWidth * 0.14f)
					* smoothRange(0.12f, 0.25f, x);
				break;
			}
			case gen_model::spaceship::PaintPattern::Hazard: {
				const float rearArmor = 1.0f - smoothRange(0.43f, 0.55f, localV);
				const float outerArmor = smoothRange(
					0.32f - secondaryWidth * 0.12f,
					0.39f - secondaryWidth * 0.08f,
					x
				);
				result.secondary = std::max(rearArmor, outerArmor);
				const float warningChevron = 0.26f + x * 0.58f;
				result.accent = gaussianBand(
					localV - warningChevron,
					0.012f + accentWidth * 0.11f
				) * smoothRange(0.08f, 0.18f, x);
				break;
			}
		}
		result.secondary = std::clamp(result.secondary, 0.0f, 1.0f);
		result.accent = std::clamp(result.accent, 0.0f, 1.0f);
		return result;
	}

	TextureFeatures sampleTextureFeatures(
		float u,
		float v,
		const gen_model::spaceship::Settings& settings
	) {
		const int region = atlasRegion(u, v);
		const float localU = localAtlasCoordinate(u);
		const float localV = localAtlasCoordinate(v);
		const PaintMasks paint = region == 0
			? paintMasks(localU, localV, settings.material)
			: PaintMasks{};
		const float detailScale = settings.material.detailScale;
		// The panel lattice is a deliberate macro feature: it gives the normal and
		// albedo maps the same broad pressure-panel language instead of unrelated noise.
		// Keep the panel rhythm broad and irregular.  A dense orthogonal grid reads as
		// a tiled sci-fi floor; large staggered service plates read as stressed metal.
		const float panelPhase = std::floor(localV * 2.5f * detailScale) * 0.217f;
		const float panelUValue = std::fmod(localU * 3.2f * detailScale + panelPhase, 1.0f);
		const float panelVValue = std::fmod(localV * 2.5f * detailScale, 1.0f);
		const float panelU = std::min(panelUValue, 1.0f - panelUValue);
		const float panelV = std::min(panelVValue, 1.0f - panelVValue);
		const float edgeDistance = std::min(panelU, panelV);
		// The player skin is one continuous recently serviced coating.  Do not let
		// the atlas' synthetic service-cell proxy become visible as a panel grid;
		// wear on this ship is reserved for any future true feature masks.
		const float edgeMask = settings.id == "player"
			? 0.0f
			: 1.0f - smoothStep(std::clamp(edgeDistance / 0.045f, 0.0f, 1.0f));
		const float macroWarpU = u + (valueNoise(
			u * 1.7f,
			v * 1.4f,
			settings.seed + 4021u
		) - 0.5f) * 0.10f;
		const float macroWarpV = v + (valueNoise(
			u * 1.5f,
			v * 1.9f,
			settings.seed + 4079u
		) - 0.5f) * 0.10f;
		const float macroCoordinateU = macroWarpU;
		const float macroCoordinateV = macroWarpV;
		const float broadNoise = valueNoise(
			macroCoordinateU * 3.2f,
			macroCoordinateV * 2.6f,
			settings.seed + 1709u
		);
		const float flow = 0.5f + 0.5f * std::sin(
			(macroCoordinateV * 3.0f + macroCoordinateU * 0.55f) * PI * 2.0f
		);
		const float macro = std::clamp(broadNoise * 0.70f + flow * 0.30f, 0.0f, 1.0f);
		const float microNoise = valueNoise(
			u * 64.0f * detailScale,
			v * 58.0f * detailScale,
			settings.seed + 2711u
		);
		const float scratchNoise = valueNoise(
			u * 220.0f * detailScale + v * 18.0f,
			v * 17.0f * detailScale - u * 3.0f,
			settings.seed + 2797u
		);
		const float micro = std::clamp(microNoise * 0.58f + scratchNoise * 0.42f, 0.0f, 1.0f);
		// A directional, low-contrast brush pass gives the coating a rolled/milled
		// steel character.  A slight rotation keeps the grain from becoming a set of
		// vertical atlas stripes while remaining elongated like a manufactured sheet.
		const float brushedBroad = valueNoise(
			u * 138.0f * detailScale + v * 21.0f,
			v * 10.0f * detailScale - u * 4.0f,
			settings.seed + 3121u
		);
		const float brushedFine = valueNoise(
			u * 330.0f * detailScale + v * 48.0f,
			v * 24.0f * detailScale - u * 6.0f,
			settings.seed + 3271u
		);
		const float brushed = std::clamp(brushedBroad * 0.78f + brushedFine * 0.22f, 0.0f, 1.0f);
		// Real pressure skins use a few long load-path breaks and service plates,
		// not a repeated orthogonal lattice.  Keep these two swept seams sparse and
		// derive them from the same field used by the height map below.  The player
		// gets the same physical language at reduced contrast; it is not a tiled
		// decal grid.
		const float seamWarp = (valueNoise(
			localU * 1.7f,
			localV * 1.4f,
			settings.seed + 4199u
		) - 0.5f) * 0.040f;
		const float spineCenter = 0.5f
			+ 0.045f * std::sin((localV * 1.35f + 0.12f) * PI)
			+ seamWarp;
		const float spineSeam = gaussianBand(localU - spineCenter, 0.014f)
			* smoothRange(0.16f, 0.78f, localV);
		const float shoulderCenter = 0.72f
			- 0.11f * std::abs(localU - 0.5f)
			+ 0.035f * std::sin((localU + seamWarp) * PI * 1.6f);
		const float shoulderWeight = smoothRange(0.13f, 0.28f, std::abs(localU - 0.5f));
		const float shoulderSeam = gaussianBand(localV - shoulderCenter, 0.017f)
			* shoulderWeight * smoothRange(0.10f, 0.92f, localV);
		const float panelSeam = region == 0
			? std::clamp(spineSeam * 0.62f + shoulderSeam * 0.78f, 0.0f, 1.0f)
			: 0.0f;
		// Functional detail is directional rather than orthogonal tiling.  A broad
		// louver/radiator rhythm belongs on dark service and engine islands, where it
		// reads as heat rejection hardware inspired by real spacecraft thermal-control
		// panels, not as a decorative grid stamped across the pressure skin.
		const bool thermalSurface = region == 1 || region == 3;
		const float atlasEdgeDistance = std::min(
			std::min(localU, 1.0f - localU),
			std::min(localV, 1.0f - localV)
		);
		const float thermalFade = thermalSurface
			? smoothRange(0.08f, 0.22f, atlasEdgeDistance)
			: 0.0f;
		const float louverWave = 0.5f + 0.5f * std::sin(
			(localU * 2.8f * detailScale + localV * 0.38f
				+ std::sin(localV * PI * 2.0f) * 0.08f) * PI * 2.0f
		);
		const float vent = thermalSurface
			? 0.5f + thermalFade * (smoothRange(0.57f, 0.93f, louverWave) - 0.5f)
			: 0.5f;
		const float thermalDetail = thermalSurface
			? thermalFade * std::abs(vent - 0.5f) * 2.0f
			: 0.0f;
		const float rustNoise = valueNoise(
			localU * 88.0f,
			localV * 72.0f,
			settings.seed + 3917u + static_cast<std::uint32_t>(region * 79)
		);
		// Rust is an engineering consequence, not a global speckle pass.  It needs a
		// crevice, a place for water/condensation to sit, or a fastener that breaks
		// the coating.  The atlas seam is our deterministic panel-feature proxy:
		// prefer the lower seam (drainage trap) and tiny panel-corner fastener halos,
		// then suppress exposed upper/service surfaces that crews reach frequently.
		const float lowerDrainage = 1.0f - smoothStep(std::clamp(panelVValue / 0.18f, 0.0f, 1.0f));
		const float cornerDistance = std::sqrt(
			std::min(panelUValue, 1.0f - panelUValue) * std::min(panelUValue, 1.0f - panelUValue)
			+ std::min(panelVValue, 1.0f - panelVValue) * std::min(panelVValue, 1.0f - panelVValue)
		);
		const float fastenerHalo = 1.0f - smoothStep(std::clamp(cornerDistance / 0.060f, 0.0f, 1.0f));
		const float accessibleSurface = 1.0f - smoothStep(std::clamp((localV - 0.52f) / 0.34f, 0.0f, 1.0f)) * 0.62f;
		const float heatZone = region == 1
			? 1.0f - smoothStep(std::clamp(localV / 0.36f, 0.0f, 1.0f))
			: 0.0f;
		const float heatExposure = 1.0f + heatZone * settings.wear.heatStaining * 0.45f;
		const float engineeringRustSite = std::clamp(
			edgeMask * (lowerDrainage * 0.78f + fastenerHalo * 0.42f) * accessibleSurface * heatExposure,
			0.0f,
			1.0f
		);
		const bool rustSurface = region == 0 || region == 1;
		const float rustThreshold = 0.992f - std::clamp(settings.wear.oxidation, 0.0f, 1.0f) * 0.060f;
		const float rustAvailability = std::clamp(settings.wear.oxidation * 2.2f, 0.0f, 1.0f);
		const float rust = rustSurface
			? engineeringRustSite * smoothRange(rustThreshold, 1.0f, rustNoise) * rustAvailability
			: 0.0f;
		return {
			region,
			localU,
			localV,
			paint.secondary,
			paint.accent,
			edgeMask,
			panelSeam,
			macro,
			micro,
			brushed,
			vent,
			thermalDetail,
			rust
		};
	}

	float textureHeight(float u, float v, const gen_model::spaceship::Settings& settings) {
		const TextureFeatures features = sampleTextureFeatures(u, v, settings);
		const bool cleanPlayerSkin = settings.id == "player";
		// Keep the player coating clean, but not visually flat: these continuous
		// relief bands are the rolled/milled metal grain that replaces a tiled panel
		// grid.  Their amplitudes are large enough for the runtime normal sampler to
		// register at game distance while remaining far below a structural ridge.
		const float macroRelief = features.region == 0
			? (cleanPlayerSkin ? 0.065f : 0.040f)
			: features.region == 1 ? 0.008f
			: features.region == 2 ? 0.002f
			: 0.006f;
		const float microRelief = features.region == 0
			? (cleanPlayerSkin ? 0.018f : 0.010f)
			: features.region == 1 ? 0.003f
			: 0.0015f;
		const float brushRelief = features.region == 0
			? (cleanPlayerSkin ? 0.020f : 0.010f)
			: features.region == 1 ? 0.003f
			: 0.0015f;
		const float panelRelief = features.region == 0
			? (cleanPlayerSkin ? 0.012f : 0.010f)
			: 0.0f;
		const float ventRelief = features.region == 1 ? 0.012f
			: features.region == 3 ? 0.007f
			: 0.0f;
		// The feature mask still drives sparse oxidation and paint loss.  Only the
		// sparse swept pressure seams receive a shallow recessed relief; a straight
		// UV lattice made the atlas read like floor tiles instead of a continuous skin.
		return -features.panelSeam * panelRelief
			+ (features.macro - 0.5f) * macroRelief
			+ (features.micro - 0.5f) * microRelief
			+ (features.brushed - 0.5f) * brushRelief
			+ (features.vent - 0.5f) * ventRelief
			+ features.rust * 0.008f;
	}

	std::array<float, 3> colorChannels(gen_model::spaceship::RgbColor color) {
		return {
			static_cast<float>(color.r),
			static_cast<float>(color.g),
			static_cast<float>(color.b)
		};
	}

	void blendColor(
		std::array<float, 3>& color,
		gen_model::spaceship::RgbColor target,
		float amount
	) {
		amount = std::clamp(amount, 0.0f, 1.0f);
		const auto targetChannels = colorChannels(target);
		for (std::size_t channel = 0; channel < color.size(); ++channel)
			color[channel] += (targetChannels[channel] - color[channel]) * amount;
	}

	std::array<std::uint8_t, 3> albedoAt(
		int column,
		int row,
		const gen_model::spaceship::Settings& settings,
		const TextureFeatures& features
	) {
		const int region = features.region;
		const float localU = features.localU;
		const float localV = features.localV;
		const float grain = hashUnit(column, row, settings.seed + 701u);
		std::array<float, 3> color{};
		if (region == 0) {
			color = colorChannels(settings.material.armorBase);
			blendColor(color, settings.material.armorSecondary, features.secondaryPaint);
			blendColor(color, settings.material.accent, features.accentPaint);
		}
		else if (region == 1)
			color = colorChannels(settings.material.structure);
		else if (region == 2)
			color = colorChannels(settings.material.canopy);
		else
			color = colorChannels(settings.material.engine);

		const bool cleanPlayerSkin = settings.id == "player";
		const float macroTone = (features.macro - 0.5f) * (
			region == 0 ? (cleanPlayerSkin ? 21.0f : 23.0f)
			: region == 1 ? 7.0f
			: region == 2 ? 2.0f
			: 5.0f
		);
		const float microTone = (features.micro - 0.5f) * (
			region == 0 ? 5.0f : region == 1 ? 2.0f : 1.0f
		);
		const float brushedTone = (features.brushed - 0.5f) * (
			region == 0 ? (cleanPlayerSkin ? 8.0f : 6.0f)
			: region == 1 ? 2.0f
			: 1.0f
		);
		const float panelTone = features.panelSeam * (region == 0 ? -8.0f : 0.0f);
		const float ventTone = (features.vent - 0.5f) * (region == 1 ? -10.0f : region == 3 ? -6.0f : 0.0f);
		color[0] += macroTone * 0.82f + microTone;
		color[1] += macroTone * 0.94f + microTone;
		color[2] += macroTone * 1.10f + microTone;
		color[0] += brushedTone * 0.86f;
		color[1] += brushedTone * 0.94f;
		color[2] += brushedTone;
		color[0] += panelTone;
		color[1] += panelTone;
		color[2] += panelTone;
		color[0] += ventTone;
		color[1] += ventTone;
		color[2] += ventTone;
		// Do not darken every synthetic panel boundary.  The normal map carries
		// only broad material relief, while the sparse feature mask remains available
		// to place believable edge wear and oxidation.
		const float paintNoise = valueNoise(
			localU * 28.0f,
			localV * 28.0f,
			settings.seed + 4783u + static_cast<std::uint32_t>(region * 83)
		);
		const float paintThreshold = 0.995f - settings.wear.paintLoss * 0.10f;
		const float paintLoss = region == 0
			? features.edgeMask * smoothRange(paintThreshold, 1.0f, paintNoise)
			: 0.0f;
		if (paintLoss > 0.0f) {
			const std::array<float, 3> exposedMetal{82.0f, 94.0f, 102.0f};
			for (std::size_t channel = 0; channel < color.size(); ++channel)
				color[channel] = color[channel] * (1.0f - paintLoss * 0.65f)
					+ exposedMetal[channel] * paintLoss * 0.65f;
		}
		if (features.rust > 0.0f) {
			const std::array<float, 3> oxide{106.0f, 68.0f, 43.0f};
			const float rustAmount = features.rust * 0.72f;
			for (std::size_t channel = 0; channel < color.size(); ++channel)
				color[channel] = color[channel] * (1.0f - rustAmount) + oxide[channel] * rustAmount;
		}
		if (region == 3) {
			const float centerDistance = std::sqrt((localU - 0.5f) * (localU - 0.5f) + (localV - 0.5f) * (localV - 0.5f));
			const float glow = std::clamp(1.0f - centerDistance * 2.0f, 0.0f, 1.0f);
			color[0] += glow * 70.0f;
			color[1] += glow * 82.0f;
			color[2] += glow * 92.0f;
			const float heat = settings.wear.heatStaining * (1.0f - localV);
			color[0] += heat * 38.0f;
			color[2] -= heat * 24.0f;
		}
		const float variation = (grain - 0.5f) * (region == 2 ? 0.8f : 1.0f);
		for (float& channel : color)
			channel = std::clamp(channel + variation, 0.0f, 255.0f);
		return {
			static_cast<std::uint8_t>(color[0]),
			static_cast<std::uint8_t>(color[1]),
			static_cast<std::uint8_t>(color[2])
		};
	}

	gen_model::spaceship::MaterialDetailReport generateTextures(
		gen_model::gen_types::AssetData& asset,
		const gen_model::spaceship::Settings& settings
	) {
		const std::size_t texelCount = static_cast<std::size_t>(settings.textureWidth) * static_cast<std::size_t>(settings.textureHeight);
		asset.texture = {settings.textureWidth, settings.textureHeight, std::vector<std::uint8_t>(texelCount * 4)};
		asset.normalMap = {settings.textureWidth, settings.textureHeight, std::vector<std::uint8_t>(texelCount * 4)};
		std::array<std::size_t, 256> slopeHistogram{};
		double slopeSum = 0.0;
		double secondarySum = 0.0;
		double accentSum = 0.0;
		double thermalSum = 0.0;
		std::size_t armorTexels = 0u;
		std::size_t thermalTexels = 0u;
		float maximumSlope = 0.0f;
		for (int row = 0; row < settings.textureHeight; ++row) {
			for (int column = 0; column < settings.textureWidth; ++column) {
				const std::size_t offset = static_cast<std::size_t>(row * settings.textureWidth + column) * 4;
				const float u = static_cast<float>(column) / static_cast<float>(settings.textureWidth - 1);
				const float v = static_cast<float>(row) / static_cast<float>(settings.textureHeight - 1);
				const TextureFeatures features = sampleTextureFeatures(u, v, settings);
				const auto albedo = albedoAt(column, row, settings, features);
				asset.texture.rgba[offset] = albedo[0];
				asset.texture.rgba[offset + 1] = albedo[1];
				asset.texture.rgba[offset + 2] = albedo[2];
				asset.texture.rgba[offset + 3] = 255;

				// Sample the continuous height field at a fixed UV distance.  Using one
				// output texel as the finite-difference step made relief shrink whenever
				// texture resolution increased: the 1024px production normal maps were
				// therefore almost flat even though reduced test maps looked embossed.
				constexpr float NORMAL_SAMPLE_STEP = 1.0f / 256.0f;
				const float leftU = std::max(0.0f, u - NORMAL_SAMPLE_STEP);
				const float rightU = std::min(1.0f, u + NORMAL_SAMPLE_STEP);
				const float lowerV = std::max(0.0f, v - NORMAL_SAMPLE_STEP);
				const float upperV = std::min(1.0f, v + NORMAL_SAMPLE_STEP);
				const float slopeX = (textureHeight(rightU, v, settings)
					- textureHeight(leftU, v, settings))
					/ std::max(rightU - leftU, EPSILON);
				const float slopeY = (textureHeight(u, upperV, settings)
					- textureHeight(u, lowerV, settings))
					/ std::max(upperV - lowerV, EPSILON);
				const Point3 normal = gen_model::gen_types::normalize({
					-slopeX * settings.material.normalStrength,
					-slopeY * settings.material.normalStrength,
					1.0f
				});
				const float normalSlope = std::clamp(
					std::sqrt(normal.x * normal.x + normal.y * normal.y),
					0.0f,
					1.0f
				);
				const auto slopeBin = static_cast<std::size_t>(std::lround(normalSlope * 255.0f));
				++slopeHistogram[std::min(slopeBin, slopeHistogram.size() - 1u)];
				slopeSum += normalSlope;
				maximumSlope = std::max(maximumSlope, normalSlope);
				if (features.region == 0) {
					secondarySum += features.secondaryPaint;
					accentSum += features.accentPaint;
					++armorTexels;
				}
				if (features.region == 1 || features.region == 3) {
					thermalSum += features.thermalDetail;
					++thermalTexels;
				}
				asset.normalMap.rgba[offset] = static_cast<std::uint8_t>(std::clamp((normal.x * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
				asset.normalMap.rgba[offset + 1] = static_cast<std::uint8_t>(std::clamp((normal.y * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
				asset.normalMap.rgba[offset + 2] = static_cast<std::uint8_t>(std::clamp((normal.z * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
				asset.normalMap.rgba[offset + 3] = 255;
			}
		}
		const std::size_t percentileTarget = static_cast<std::size_t>(
			std::ceil(static_cast<double>(texelCount) * 0.95)
		);
		std::size_t cumulative = 0u;
		float percentile95 = 0.0f;
		for (std::size_t bin = 0; bin < slopeHistogram.size(); ++bin) {
			cumulative += slopeHistogram[bin];
			if (cumulative < percentileTarget)
				continue;
			percentile95 = static_cast<float>(bin) / 255.0f;
			break;
		}
		return {
			static_cast<float>(slopeSum / static_cast<double>(texelCount)),
			percentile95,
			maximumSlope,
			armorTexels == 0u ? 0.0f : static_cast<float>(secondarySum / static_cast<double>(armorTexels)),
			armorTexels == 0u ? 0.0f : static_cast<float>(accentSum / static_cast<double>(armorTexels)),
			thermalTexels == 0u ? 0.0f : static_cast<float>(thermalSum / static_cast<double>(thermalTexels))
		};
	}

	float pointTriangleDistanceSquared(Point3 point, Point3 a, Point3 b, Point3 c) {
		const Point3 ab = b - a;
		const Point3 ac = c - a;
		const Point3 ap = point - a;
		const float d1 = gen_model::gen_types::dot(ab, ap);
		const float d2 = gen_model::gen_types::dot(ac, ap);
		if (d1 <= 0.0f && d2 <= 0.0f)
			return lengthSquared(ap);
		const Point3 bp = point - b;
		const float d3 = gen_model::gen_types::dot(ab, bp);
		const float d4 = gen_model::gen_types::dot(ac, bp);
		if (d3 >= 0.0f && d4 <= d3)
			return lengthSquared(bp);
		const float vc = d1 * d4 - d3 * d2;
		if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
			const float amount = d1 / (d1 - d3);
			return lengthSquared(point - (a + ab * amount));
		}
		const Point3 cp = point - c;
		const float d5 = gen_model::gen_types::dot(ab, cp);
		const float d6 = gen_model::gen_types::dot(ac, cp);
		if (d6 >= 0.0f && d5 <= d6)
			return lengthSquared(cp);
		const float vb = d5 * d2 - d1 * d6;
		if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
			const float amount = d2 / (d2 - d6);
			return lengthSquared(point - (a + ac * amount));
		}
		const float va = d3 * d6 - d5 * d4;
		if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
			const Point3 edge = c - b;
			const float amount = (d4 - d3) / ((d4 - d3) + (d5 - d6));
			return lengthSquared(point - (b + edge * amount));
		}
		const float denominator = 1.0f / (va + vb + vc);
		const float v = vb * denominator;
		const float w = vc * denominator;
		return lengthSquared(point - (a + ab * v + ac * w));
	}

	float segmentSegmentDistanceSquared(Point3 p1, Point3 q1, Point3 p2, Point3 q2) {
		const Point3 d1 = q1 - p1;
		const Point3 d2 = q2 - p2;
		const Point3 r = p1 - p2;
		const float a = gen_model::gen_types::dot(d1, d1);
		const float e = gen_model::gen_types::dot(d2, d2);
		const float f = gen_model::gen_types::dot(d2, r);
		float s = 0.0f;
		float t = 0.0f;
		if (a <= EPSILON && e <= EPSILON)
			return lengthSquared(p1 - p2);
		if (a <= EPSILON) {
			t = std::clamp(f / e, 0.0f, 1.0f);
		} else {
			const float c = gen_model::gen_types::dot(d1, r);
			if (e <= EPSILON) {
				s = std::clamp(-c / a, 0.0f, 1.0f);
			} else {
				const float b = gen_model::gen_types::dot(d1, d2);
				const float denominator = a * e - b * b;
				if (denominator != 0.0f)
					s = std::clamp((b * f - c * e) / denominator, 0.0f, 1.0f);
				t = (b * s + f) / e;
				if (t < 0.0f) {
					t = 0.0f;
					s = std::clamp(-c / a, 0.0f, 1.0f);
				} else if (t > 1.0f) {
					t = 1.0f;
					s = std::clamp((b - c) / a, 0.0f, 1.0f);
				}
			}
		}
		return lengthSquared((p1 + d1 * s) - (p2 + d2 * t));
	}

	bool segmentIntersectsTriangle(Point3 start, Point3 end, Point3 a, Point3 b, Point3 c) {
		const Point3 direction = end - start;
		const Point3 edge1 = b - a;
		const Point3 edge2 = c - a;
		const Point3 p = gen_model::gen_types::cross(direction, edge2);
		const float determinant = gen_model::gen_types::dot(edge1, p);
		if (std::abs(determinant) <= EPSILON)
			return false;
		const float inverse = 1.0f / determinant;
		const Point3 t = start - a;
		const float u = gen_model::gen_types::dot(t, p) * inverse;
		if (u < 0.0f || u > 1.0f)
			return false;
		const Point3 q = gen_model::gen_types::cross(t, edge1);
		const float v = gen_model::gen_types::dot(direction, q) * inverse;
		if (v < 0.0f || u + v > 1.0f)
			return false;
		const float distanceAlong = gen_model::gen_types::dot(edge2, q) * inverse;
		return distanceAlong >= 0.0f && distanceAlong <= 1.0f;
	}

	float segmentTriangleDistanceSquared(Point3 start, Point3 end, Point3 a, Point3 b, Point3 c) {
		if (segmentIntersectsTriangle(start, end, a, b, c))
			return 0.0f;
		return std::min({
			pointTriangleDistanceSquared(start, a, b, c),
			pointTriangleDistanceSquared(end, a, b, c),
			segmentSegmentDistanceSquared(start, end, a, b),
			segmentSegmentDistanceSquared(start, end, b, c),
			segmentSegmentDistanceSquared(start, end, c, a)
		});
	}

	std::vector<Point3> sampleDirections(Point3 forward, float halfAngleDegrees) {
		forward = gen_model::gen_types::normalize(forward);
		Point3 reference = std::abs(forward.y) < 0.9f ? Point3{0.0f, 1.0f, 0.0f} : Point3{1.0f, 0.0f, 0.0f};
		const Point3 right = gen_model::gen_types::normalize(gen_model::gen_types::cross(reference, forward));
		const Point3 up = gen_model::gen_types::normalize(gen_model::gen_types::cross(forward, right));
		std::vector<Point3> directions{forward};
		directions.reserve(1u + FIRING_CONE_RINGS.size() * FIRING_CONE_AZIMUTH_SAMPLES);
		for (const float fraction : FIRING_CONE_RINGS) {
			const float angle = halfAngleDegrees * fraction * DEGREES_TO_RADIANS;
			for (int sample = 0; sample < FIRING_CONE_AZIMUTH_SAMPLES; ++sample) {
				const float azimuth = 2.0f * PI * static_cast<float>(sample)
					/ static_cast<float>(FIRING_CONE_AZIMUTH_SAMPLES);
				const Point3 radial = right * std::cos(azimuth) + up * std::sin(azimuth);
				directions.push_back(gen_model::gen_types::normalize(forward * std::cos(angle) + radial * std::sin(angle)));
			}
		}
		return directions;
	}

	float firingConeSamplingInflation(const gen_model::spaceship::MountSettings& mount) {
		// The 64x8 deterministic tessellation is intentionally dense enough to
		// inspect the entire traverse cone without making every candidate search
		// quadratic in a large artificial inflation radius.  Keep a tiny numerical
		// skin so a tangent contact is still rejected by the positive margin gate.
		return std::max(0.001f, mount.barrelLength * 0.0005f);
	}

	gen_model::spaceship::MountReport inspectMount(
		const gen_model::spaceship::Settings& settings,
		const TaggedMesh& taggedMesh,
		std::size_t mountIndex,
		const SurfaceAttachment& attachment
	) {
		const auto& mount = settings.mounts[mountIndex];
		const float parentCollisionClearance =
			gen_model::spaceship::weapon_layout::unitParentCollisionClearance(mount);
		if (parentCollisionClearance < 0.02f) {
			std::ostringstream message;
			message << "Spaceship mount enters unit parent collision sphere: " << mount.id
				<< " index=" << mountIndex
				<< " clearance=" << parentCollisionClearance
				<< " position=(" << mount.position.x << ',' << mount.position.y << ',' << mount.position.z
				<< ") turretRadius=" << mount.turretRadius;
			throw std::invalid_argument(message.str());
		}
		const Point3 normalizedForward = gen_model::gen_types::normalize(mount.forward);
		float facingErrorDegrees = 0.0f;
		if (mount.requestedFacing.has_value()) {
			const Point3 requested = gen_model::gen_types::normalize(*mount.requestedFacing);
			const float cosine = std::clamp(
				gen_model::gen_types::dot(requested, normalizedForward), -1.0f, 1.0f
			);
			facingErrorDegrees = std::acos(cosine) / DEGREES_TO_RADIANS;
			if (facingErrorDegrees > 0.5f) {
				std::ostringstream message;
				message << "Spaceship mount resolved away from requested facing: " << mount.id
					<< " index=" << mountIndex
					<< " errorDegrees=" << facingErrorDegrees;
				throw std::invalid_argument(message.str());
			}
		}
		const bool structurallyConnected = attachment.onGeneratedSurface
			&& finite(attachment.surfacePoint);
		float minimumDistance = std::numeric_limits<float>::max();
		Point3 minimumDirection{};
		Surface minimumSurface = Surface::Armor;
		int minimumOwner = -1;
		bool minimumSocket = false;
		const auto directions = sampleDirections(mount.forward, mount.traverseHalfAngleDegrees);
		const float coneInflation = firingConeSamplingInflation(mount);
		for (const Point3 direction : directions) {
			const Point3 start = mount.position + direction * (mount.turretRadius + mount.barrelRadius * 2.0f);
			const Point3 end = mount.position + direction * mount.barrelLength;
			for (std::size_t triangleIndex = 0; triangleIndex < taggedMesh.mesh.triangles.size(); ++triangleIndex) {
				const auto& tag = taggedMesh.tags[triangleIndex];
				// Socket cylinders sit below the turret sphere and are the intentional barrel pivot;
				// they are not a forward obstruction. Supports and all ship geometry remain checked.
				if (tag.socket)
					continue;
				const auto& triangle = taggedMesh.mesh.triangles[triangleIndex];
				const Point3 a = taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])];
				const Point3 b = taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])];
				const Point3 c = taggedMesh.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])];
				const float distanceSquared = segmentTriangleDistanceSquared(start, end, a, b, c);
				const float clearance = std::sqrt(distanceSquared)
					- (mount.barrelRadius + coneInflation);
				if (clearance < minimumDistance) {
					minimumDistance = clearance;
					minimumDirection = direction;
					minimumSurface = tag.surface;
					minimumOwner = tag.mountOwner;
					minimumSocket = tag.socket;
				}
			}
		}
		if (minimumDistance < FIRING_CLEARANCE_MARGIN) {
			std::ostringstream message;
			message << "Spaceship mount firing envelope intersects generated geometry: " << mount.id
				<< " index=" << mountIndex
				<< " clearance=" << minimumDistance
				<< " position=(" << mount.position.x << ',' << mount.position.y << ',' << mount.position.z << ')'
				<< " direction=(" << minimumDirection.x << ',' << minimumDirection.y << ',' << minimumDirection.z << ')'
				<< " surface=" << static_cast<int>(minimumSurface)
				<< " owner=" << minimumOwner
				<< " socket=" << minimumSocket;
			message << " attachment=(" << attachment.surfacePoint.x << ','
				<< attachment.surfacePoint.y << ',' << attachment.surfacePoint.z
				<< ") lowerTangent=(" << attachment.lowerTangent.x << ','
				<< attachment.lowerTangent.y << ',' << attachment.lowerTangent.z
				<< ") supportLength=" << attachment.supportLength
				<< " takeoffAngle=" << attachment.takeoffAngleDegrees;
			for (std::size_t triangleIndex = 0; triangleIndex < taggedMesh.tags.size(); ++triangleIndex) {
				if (taggedMesh.tags[triangleIndex].mountOwner != minimumOwner)
					continue;
				const auto& triangle = taggedMesh.mesh.triangles[triangleIndex];
				const Point3 ownerA = taggedMesh.mesh.positions[
					static_cast<std::size_t>(triangle.positionIndices[0])
				];
				const Point3 ownerB = taggedMesh.mesh.positions[
					static_cast<std::size_t>(triangle.positionIndices[1])
				];
				const Point3 ownerC = taggedMesh.mesh.positions[
					static_cast<std::size_t>(triangle.positionIndices[2])
				];
				const Point3 checkStart = mount.position
					+ minimumDirection * (mount.turretRadius + mount.barrelRadius * 2.0f);
				const Point3 checkEnd = mount.position + minimumDirection * mount.barrelLength;
				if (segmentTriangleDistanceSquared(checkStart, checkEnd, ownerA, ownerB, ownerC)
					<= (mount.barrelRadius + 0.05f) * (mount.barrelRadius + 0.05f)) {
					message << " ownerTriangle=(" << ownerA.x << ',' << ownerA.y << ',' << ownerA.z
						<< ") (" << ownerB.x << ',' << ownerB.y << ',' << ownerB.z
						<< ") (" << ownerC.x << ',' << ownerC.y << ',' << ownerC.z << ')';
					break;
				}
			}
			throw std::invalid_argument(message.str());
		}
		return {
			mount.id,
			structurallyConnected,
			std::min(mount.supportWidth, mount.supportHeight),
			minimumDistance,
			parentCollisionClearance,
			attachment.surfacePoint,
			mount.forward,
			facingErrorDegrees,
			attachment.takeoffAngleDegrees,
			attachment.supportLength,
			attachment.directBlister
		};
	}

	gen_model::spaceship::Bounds calculateBounds(const gen_model::gen_types::MeshData& mesh) {
		if (mesh.positions.empty())
			throw std::logic_error("Generated spaceship mesh is empty");
		gen_model::spaceship::Bounds bounds;
		bounds.minimum = mesh.positions.front();
		bounds.maximum = mesh.positions.front();
		for (const Point3 point : mesh.positions) {
			bounds.minimum.x = std::min(bounds.minimum.x, point.x);
			bounds.minimum.y = std::min(bounds.minimum.y, point.y);
			bounds.minimum.z = std::min(bounds.minimum.z, point.z);
			bounds.maximum.x = std::max(bounds.maximum.x, point.x);
			bounds.maximum.y = std::max(bounds.maximum.y, point.y);
			bounds.maximum.z = std::max(bounds.maximum.z, point.z);
			bounds.maximumRadius = std::max(bounds.maximumRadius, length(point));
		}
		return bounds;
	}

	void hashBytes(std::uint64_t& hash, const void* data, std::size_t size) {
		const auto* bytes = static_cast<const std::uint8_t*>(data);
		for (std::size_t index = 0; index < size; ++index) {
			hash ^= bytes[index];
			hash *= 1099511628211ull;
		}
	}

	void hashString(std::uint64_t& hash, const std::string& value) {
		hashBytes(hash, value.data(), value.size());
	}

	void hashFloat(std::uint64_t& hash, float value) {
		const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
		hashBytes(hash, &bits, sizeof(bits));
	}

	void hashPoint(std::uint64_t& hash, Point3 point) {
		hashFloat(hash, point.x);
		hashFloat(hash, point.y);
		hashFloat(hash, point.z);
	}

	void hashColor(std::uint64_t& hash, gen_model::spaceship::RgbColor color) {
		hashBytes(hash, &color.r, sizeof(color.r));
		hashBytes(hash, &color.g, sizeof(color.g));
		hashBytes(hash, &color.b, sizeof(color.b));
	}
}

std::uint64_t gen_model::spaceship::fingerprint(const gen_model::spaceship::Settings& settings) {
	std::uint64_t hash = 1469598103934665603ull;
	hashString(hash, settings.id);
	hashString(hash, settings.modelPath);
	hashBytes(hash, &settings.seed, sizeof(settings.seed));
	hashFloat(hash, settings.dimensions.width);
	hashFloat(hash, settings.dimensions.height);
	hashFloat(hash, settings.dimensions.length);
	hashFloat(hash, settings.hull.width);
	hashFloat(hash, settings.hull.height);
	hashFloat(hash, settings.hull.length);
	hashFloat(hash, settings.hull.crown);
	hashFloat(hash, settings.hull.keel);
	hashFloat(hash, settings.hull.noseSharpness);
	hashFloat(hash, settings.hull.rearTaper);
	hashFloat(hash, settings.wings.halfSpan);
	hashFloat(hash, settings.wings.rootFrontZ);
	hashFloat(hash, settings.wings.rootRearZ);
	hashFloat(hash, settings.wings.tipFrontZ);
	hashFloat(hash, settings.wings.tipRearZ);
	hashFloat(hash, settings.wings.rootX);
	hashFloat(hash, settings.wings.topY);
	hashFloat(hash, settings.wings.bottomY);
	hashFloat(hash, settings.wings.shoulderWidth);
	hashPoint(hash, settings.cockpit.center);
	hashPoint(hash, settings.cockpit.size);
	hashFloat(hash, settings.cockpit.browDepth);
	hashString(hash, settings.layout.archetype);
	hashBytes(hash, &settings.layout.crew, sizeof(settings.layout.crew));
	hashFloat(hash, settings.layout.primarySpineWidth);
	hashFloat(hash, settings.layout.reactorRadius);
	hashFloat(hash, settings.layout.serviceBayLength);
	hashFloat(hash, settings.layout.radiatorScale);
	hashFloat(hash, settings.layout.weaponDeckCantDegrees);
	hashBytes(hash, &settings.design.propulsionLayout, sizeof(settings.design.propulsionLayout));
	hashBytes(hash, &settings.design.propulsionPlacement, sizeof(settings.design.propulsionPlacement));
	hashFloat(hash, settings.design.targetAcceleration);
	hashFloat(hash, settings.design.endurance);
	hashFloat(hash, settings.design.armorMassScale);
	hashFloat(hash, settings.design.engineTechnology);
	hashFloat(hash, settings.design.moduleClearance);
	hashFloat(hash, settings.design.hardSurfaceBias);
	hashBytes(hash, &settings.design.weaponLayout.placement, sizeof(settings.design.weaponLayout.placement));
	hashBytes(hash, &settings.design.weaponLayout.coverage, sizeof(settings.design.weaponLayout.coverage));
	hashBytes(hash, &settings.design.weaponLayout.symmetry, sizeof(settings.design.weaponLayout.symmetry));
	hashBytes(hash, &settings.design.weaponLayout.batteryStyle, sizeof(settings.design.weaponLayout.batteryStyle));
	hashBytes(hash, &settings.design.weaponLayout.turretCount, sizeof(settings.design.weaponLayout.turretCount));
	hashFloat(hash, settings.design.weaponLayout.minimumSeparationScale);
	if (settings.design.propulsionPlacement == PlacementMode::Manual) {
		const std::size_t engineCount = settings.engines.size();
		hashBytes(hash, &engineCount, sizeof(engineCount));
		for (const auto& engine : settings.engines) {
			hashPoint(hash, engine.center);
			hashFloat(hash, engine.radius);
			hashFloat(hash, engine.length);
			hashFloat(hash, engine.nozzleDepth);
		}
	}
	for (const auto& mount : settings.mounts) {
		hashFloat(hash, mount.turretRadius);
		hashFloat(hash, mount.barrelRadius);
		hashFloat(hash, mount.barrelLength);
		hashFloat(hash, mount.traverseHalfAngleDegrees);
		hashFloat(hash, mount.supportWidth);
		hashFloat(hash, mount.supportHeight);
		hashFloat(hash, mount.socketHeight);
		const bool hasRequestedFacing = mount.requestedFacing.has_value();
		hashBytes(hash, &hasRequestedFacing, sizeof(hasRequestedFacing));
		if (hasRequestedFacing)
			hashPoint(hash, *mount.requestedFacing);
		if (settings.design.weaponLayout.placement == PlacementMode::Manual) {
			hashString(hash, mount.id);
			hashPoint(hash, mount.position);
			hashPoint(hash, mount.forward);
			hashPoint(hash, mount.supportRoot);
		}
	}
	hashFloat(hash, settings.mountAttachment.preferredTakeoffAngleDegrees);
	hashFloat(hash, settings.mountAttachment.minimumTakeoffAngleDegrees);
	hashFloat(hash, settings.mountAttachment.maximumTakeoffAngleDegrees);
	hashFloat(hash, settings.mountAttachment.directBlisterGapScale);
	hashFloat(hash, settings.mountAttachment.distanceWeight);
	hashFloat(hash, settings.mountAttachment.angleWeight);
	hashFloat(hash, settings.mountAttachment.blisterRadiusScale);
	hashFloat(hash, settings.wear.paintLoss);
	hashFloat(hash, settings.wear.oxidation);
	hashFloat(hash, settings.wear.heatStaining);
	hashBytes(hash, &settings.wear.repairPanels, sizeof(settings.wear.repairPanels));
	hashColor(hash, settings.material.armorBase);
	hashColor(hash, settings.material.armorSecondary);
	hashColor(hash, settings.material.accent);
	hashColor(hash, settings.material.structure);
	hashColor(hash, settings.material.canopy);
	hashColor(hash, settings.material.engine);
	hashBytes(hash, &settings.material.pattern, sizeof(settings.material.pattern));
	hashFloat(hash, settings.material.secondaryCoverage);
	hashFloat(hash, settings.material.accentCoverage);
	hashFloat(hash, settings.material.normalStrength);
	hashFloat(hash, settings.material.detailScale);
	hashBytes(hash, &settings.cylinderSegments, sizeof(settings.cylinderSegments));
	hashBytes(hash, &settings.textureWidth, sizeof(settings.textureWidth));
	hashBytes(hash, &settings.textureHeight, sizeof(settings.textureHeight));
	hashFloat(hash, settings.armorDepth);
	return hash;
}

[[maybe_unused]] gen_model::spaceship::GeneratedShip gen_model::spaceship::generateLegacy(
	const gen_model::spaceship::Settings& settings
) {
	validateSettings(settings);
	MeshBuilder builder;
	const bool broadAirframe = settings.layout.archetype == "siege_gunship"
		|| settings.layout.archetype == "carrier";
	if (isFighterArchetype(settings)) {
		addIntegratedFighterAirframe(builder, settings);
		addWings(builder, settings);
	}
	else if (broadAirframe) {
		// Capitals keep the rounded pressure hull and a separate, shallow lifting/deck
		// surface.  The wing loft is the load-bearing outer skin that the mount saddles
		// grow from; service fairings remain flush with that skin.
		addHull(builder, settings);
		addWings(builder, settings);
	}
	else {
		addHull(builder, settings);
		addWings(builder, settings);
	}
	addSystemsLayout(builder, settings);
	addCockpit(builder, settings);
	addEngines(builder, settings);
	addArmorLandmarks(builder, settings);
	const std::vector<SurfaceAttachment> attachments = addMounts(builder, settings);
	TaggedMesh taggedMesh = builder.finish();
	requireClearExhaustApertures(settings, nullptr, taggedMesh);

	GeneratedShip result;
	result.bounds = calculateBounds(taggedMesh.mesh);
	result.mounts.reserve(settings.mounts.size());
	for (std::size_t index = 0; index < settings.mounts.size(); ++index) {
		MountReport report = inspectMount(settings, taggedMesh, index, attachments[index]);
		if (!report.structurallyConnected)
			throw std::invalid_argument(
				"Spaceship mount optimizer did not connect to generated geometry: " + report.id
				+ " attachment=(" + std::to_string(report.attachmentPoint.x)
				+ "," + std::to_string(report.attachmentPoint.y)
				+ "," + std::to_string(report.attachmentPoint.z) + ")"
			);
		result.mounts.push_back(report);
	}
	result.asset.mesh = std::move(taggedMesh.mesh);
	result.materialDetails = generateTextures(result.asset, settings);
	result.settingsFingerprint = fingerprint(settings);
	return result;
}

gen_model::spaceship::GeneratedShip gen_model::spaceship::generate(
	const gen_model::spaceship::Settings& settings
) {
	validateSettings(settings);
	const auto preliminary = design::planCore(settings);
	const auto candidates = weapon_layout::plan(settings, preliminary);
	std::vector<std::string> failures;
	for (const auto& candidate : candidates.ranked) {
		try {
			const auto plan = design::complete(
				settings,
				preliminary,
				candidate.modules,
				candidate.mounts
			);
			design::requireValid(settings, plan);
			Settings resolved = settings;
			resolved.mounts = candidate.mounts;
			resolved.engines.clear();
			resolved.engines.reserve(plan.enginePods.size());
			for (const auto& pod : plan.enginePods)
				resolved.engines.push_back(pod.runtime);

			MeshBuilder builder;
			const bool broadAirframe = resolved.layout.archetype == "siege_gunship"
				|| resolved.layout.archetype == "carrier";
			if (isFighterArchetype(resolved)) {
				addIntegratedFighterAirframe(builder, resolved);
				addWings(builder, resolved);
			} else if (broadAirframe) {
				addHull(builder, resolved);
				addWings(builder, resolved);
			} else {
				addHull(builder, resolved);
				addWings(builder, resolved);
			}
			addSystemsLayout(builder, resolved);
			addCockpit(builder, resolved);
			addEngines(builder, resolved, &plan.enginePods);
			const SystemDetailReport systemDetails =
				envelope::appendFunctionalFairings(builder, resolved, plan);
			addArmorLandmarks(builder, resolved);
			const auto attachments = addMounts(builder, resolved);
			TaggedMesh taggedMesh = builder.finish();
			requireClearExhaustApertures(resolved, &plan.enginePods, taggedMesh);

			GeneratedShip result;
			result.bounds = calculateBounds(taggedMesh.mesh);
			result.mounts.reserve(resolved.mounts.size());
			for (std::size_t index = 0; index < resolved.mounts.size(); ++index) {
				const MountReport report = inspectMount(
					resolved,
					taggedMesh,
					index,
					attachments[index]
				);
				if (!report.structurallyConnected)
					throw std::invalid_argument(
						"Automatic weapon layout produced an unsupported mount: " + report.id
					);
				result.mounts.push_back(report);
			}
			result.asset.mesh = std::move(taggedMesh.mesh);
			result.materialDetails = generateTextures(result.asset, resolved);
			result.resolvedEngines = resolved.engines;
			result.resolvedNozzleCells.reserve(plan.enginePods.size());
			for (const auto& pod : plan.enginePods)
				result.resolvedNozzleCells.push_back(pod.nozzleCells);
			result.resolvedMounts = resolved.mounts;
			result.design = plan.metrics;
			result.systemDetails = systemDetails;
			result.settingsFingerprint = fingerprint(settings);
			return result;
		} catch (const std::exception& error) {
			failures.push_back(error.what());
		}
	}
	std::ostringstream message;
	message << "No automatic spaceship candidate passed the design/clearance gates: " << settings.id;
	for (const auto& failure : failures)
		message << " [" << failure << ']';
	throw std::invalid_argument(message.str());
}
