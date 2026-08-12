#include "spaceship_design.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>

namespace {
	constexpr float PI = 3.14159265358979323846f;
	constexpr float EPSILON = 0.00001f;
	constexpr int MASS_SOLVE_ITERATIONS = 8;

	using Point3 = gen_model::gen_types::Point3;
	using Settings = gen_model::spaceship::Settings;
	using ModuleKind = gen_model::spaceship::design::ModuleKind;
	using ModuleShape = gen_model::spaceship::design::ModuleShape;
	using PropulsionLayout = gen_model::spaceship::PropulsionLayout;
	using PlacementMode = gen_model::spaceship::PlacementMode;
	using EngineSettings = gen_model::spaceship::EngineSettings;

	float sphereVolume(float radius) {
		return 4.0f * PI * radius * radius * radius / 3.0f;
	}

	float boxVolume(Point3 halfExtents) {
		return 8.0f * halfExtents.x * halfExtents.y * halfExtents.z;
	}

	float ellipsoidVolume(Point3 halfExtents) {
		return 4.0f * PI * halfExtents.x * halfExtents.y * halfExtents.z / 3.0f;
	}

	float cylinderVolume(Point3 halfExtents) {
		return PI * halfExtents.x * halfExtents.y * halfExtents.z * 2.0f;
	}

	float shapeVolume(ModuleShape shape, Point3 extents) {
		switch (shape) {
			case ModuleShape::FacetedCapsule:
			case ModuleShape::Ellipsoid:
			case ModuleShape::ShieldedSphere:
				return ellipsoidVolume(extents);
			case ModuleShape::CappedCylinder:
			case ModuleShape::AxialFrustum:
				return cylinderVolume(extents);
			default:
				return boxVolume(extents);
		}
	}

	bool finitePoint(Point3 point) {
		return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
	}

	bool fitShapeVolume(ModuleShape shape, Point3& extents, float requiredVolume) {
		if (!finitePoint(extents) || extents.x <= EPSILON || extents.y <= EPSILON || extents.z <= EPSILON)
			return false;
		if (!std::isfinite(requiredVolume) || requiredVolume < 0.0f)
			return false;
		const float representedVolume = shapeVolume(shape, extents);
		if (!std::isfinite(representedVolume) || representedVolume <= EPSILON)
			return requiredVolume <= EPSILON;
		if (representedVolume >= requiredVolume)
			return true;
		const float scale = std::cbrt(requiredVolume / representedVolume) * 1.000001f;
		if (!std::isfinite(scale) || scale <= EPSILON)
			return false;
		extents = extents * scale;
		const float fittedVolume = shapeVolume(shape, extents);
		return finitePoint(extents) && std::isfinite(fittedVolume)
			&& fittedVolume + EPSILON >= requiredVolume;
	}

	float weaponBurden(const Settings& settings) {
		float result = 0.0f;
		for (const auto& mount : settings.mounts) {
			result += sphereVolume(mount.turretRadius) * 7.0f;
			result += PI * mount.barrelRadius * mount.barrelRadius * mount.barrelLength * 3.0f;
		}
		return result;
	}

	float weaponPower(const Settings& settings) {
		float result = 0.0f;
		for (const auto& mount : settings.mounts)
			result += mount.turretRadius * mount.turretRadius * mount.barrelLength;
		return result;
	}

	PropulsionLayout chooseLayout(const Settings& settings, float burden) {
		if (settings.design.propulsionLayout != PropulsionLayout::Auto)
			return settings.design.propulsionLayout;
		if (settings.layout.crew >= 64)
			return PropulsionLayout::CapitalSideBlocks;
		if (settings.mounts.size() >= 16 || burden > 6.0f)
			return PropulsionLayout::DistributedAft;
		const float widthToLength = settings.dimensions.width / settings.dimensions.length;
		if (settings.design.targetAcceleration >= 1.35f)
			return PropulsionLayout::TwinBoom;
		if (widthToLength >= 1.35f)
			return PropulsionLayout::WingNacelles;
		if (settings.engines.size() >= 3)
			return PropulsionLayout::SpineCluster;
		return PropulsionLayout::CentralCluster;
	}

	struct Performance {
		float mass = 0.0f;
		float requiredThrust = 0.0f;
		float engineVolume = 0.0f;
		float fuelVolume = 0.0f;
		float reactorVolume = 0.0f;
	};

	Performance solvePerformance(
		const Settings& settings,
		float burden,
		float reactorBase
	) {
		const float structureMass = settings.dimensions.width
			* settings.dimensions.height * settings.dimensions.length * 0.11f;
		const float cockpitMass = settings.cockpit.size.x
			* settings.cockpit.size.y * settings.cockpit.size.z * 0.55f;
		const float crewMass = static_cast<float>(std::max(settings.layout.crew, 1)) * 0.08f;
		const float dryMass = structureMass * settings.design.armorMassScale
			+ cockpitMass + crewMass + burden * 0.32f + reactorBase * 0.55f;
		Performance result{dryMass};
		const float technology = std::max(settings.design.engineTechnology, 0.25f);
		for (int iteration = 0; iteration < MASS_SOLVE_ITERATIONS; ++iteration) {
			result.requiredThrust = result.mass * std::max(settings.design.targetAcceleration, 0.05f);
			result.engineVolume = result.requiredThrust / (technology * 4.8f);
			result.fuelVolume = result.requiredThrust * std::max(settings.design.endurance, 0.05f)
				/ (technology * 3.6f);
			const float enginePower = result.requiredThrust * 0.12f;
			result.reactorVolume = std::max(enginePower, weaponPower(settings)) / 2.8f
				+ reactorBase * 0.42f;
			result.mass = dryMass + result.engineVolume * 0.20f
				+ result.fuelVolume * 0.08f + result.reactorVolume * 0.18f;
		}
		return result;
	}

	ModuleShape defaultShape(ModuleKind kind, std::uint32_t seed) {
		const bool alternate = ((seed * 1664525u + 1013904223u) >> 28u & 1u) != 0u;
		switch (kind) {
			case ModuleKind::Cockpit:
				return alternate ? ModuleShape::ArmoredWedge : ModuleShape::FacetedCapsule;
			case ModuleKind::FuelTank:
				return alternate ? ModuleShape::CappedCylinder : ModuleShape::Ellipsoid;
			case ModuleKind::Reactor:
				return alternate ? ModuleShape::CappedCylinder : ModuleShape::ShieldedSphere;
			case ModuleKind::Magazine:
				return alternate ? ModuleShape::TaperedBox : ModuleShape::ChamferedBox;
			case ModuleKind::EngineCore:
				return alternate ? ModuleShape::CappedCylinder : ModuleShape::AxialFrustum;
			default:
				return alternate ? ModuleShape::TaperedBeam : ModuleShape::ChamferedBox;
		}
	}

	gen_model::spaceship::design::ModuleVolume module(
		ModuleKind kind,
		ModuleShape shape,
		Point3 center,
		Point3 extents,
		float requiredVolume,
		float mass,
		int ownerIndex,
		bool protectedByEnvelope = true
	) {
		if (!fitShapeVolume(shape, extents, requiredVolume))
			throw std::invalid_argument("Spaceship module cannot be packaged in the requested shape");
		const float actualVolume = shapeVolume(shape, extents);
		return {
			kind,
			shape,
			center,
			extents,
			{0.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 1.0f},
			{0.0f, -1.0f, 0.0f},
			requiredVolume,
			actualVolume,
			std::min({extents.x, extents.y, extents.z}) * 0.18f,
			mass,
			ownerIndex,
			protectedByEnvelope
		};
	}

	Point3 engineCenter(
		const Settings& settings,
		PropulsionLayout layout,
		std::size_t index,
		std::size_t count,
		float radius
	) {
		const float centered = static_cast<float>(index)
			- static_cast<float>(count - 1u) * 0.5f;
		const float side = centered == 0.0f ? 0.0f : std::copysign(1.0f, centered);
		const float absSide = std::abs(centered);
		const float lateral = std::max(
			settings.hull.width * 0.48f,
			settings.wings.rootX + radius * 1.8f
		);
		const float z = -settings.hull.length * 0.34f
			+ (count > 2u ? -absSide * radius * 0.45f : 0.0f);
		switch (layout) {
			case PropulsionLayout::WingNacelles:
				return {side * lateral, 0.0f, z};
			case PropulsionLayout::TwinBoom:
				return {side * lateral * 0.92f, 0.0f, z * 0.92f};
			case PropulsionLayout::CapitalSideBlocks:
			case PropulsionLayout::DistributedAft:
			{
				const float sideRank = std::max(absSide - 0.5f, 0.0f);
				const float lateralBase = std::max(
					settings.hull.width * 0.58f,
					radius * 2.4f
				);
				const float machineryPitch = radius * 2.8f
					+ settings.design.moduleClearance * 1.5f;
				return {
					side * (lateralBase + sideRank * machineryPitch),
					-sideRank * radius * 0.22f,
					z - sideRank * radius * 0.55f
				};
			}
			case PropulsionLayout::SpineCluster:
				return {
					centered * std::max(radius * 3.6f, settings.hull.width * 0.48f),
					0.0f,
					z
				};
			case PropulsionLayout::CentralCluster:
			default:
				return {centered * radius * 1.65f, 0.0f, z};
		}
	}

	std::size_t automaticPodCount(PropulsionLayout layout) {
		switch (layout) {
			case PropulsionLayout::CentralCluster:
				return 2u;
			case PropulsionLayout::SpineCluster:
				return 3u;
			case PropulsionLayout::TwinBoom:
			case PropulsionLayout::WingNacelles:
				return 2u;
			case PropulsionLayout::DistributedAft:
				return 6u;
			case PropulsionLayout::CapitalSideBlocks:
				return 4u;
			case PropulsionLayout::Auto:
				return 2u;
		}
		return 2u;
	}

	std::size_t radiatorPairCount(
		const Settings& settings,
		float reactorRadius
	) {
		const float demand = settings.layout.radiatorScale * reactorRadius
			+ static_cast<float>(settings.mounts.size()) * 0.030f;
		return static_cast<std::size_t>(std::clamp(
			static_cast<int>(std::ceil(demand)), 1, 4
		));
	}

	std::vector<gen_model::spaceship::design::EnginePod> resolveEngines(
		const Settings& settings,
		PropulsionLayout layout,
		const Performance& performance
	) {
		if (settings.engines.empty())
			return {};
		const std::size_t count = settings.design.propulsionPlacement == PlacementMode::Manual
			? settings.engines.size()
			: automaticPodCount(layout);
		std::vector<gen_model::spaceship::design::EnginePod> result;
		result.reserve(count);
		const float technology = std::max(settings.design.engineTechnology, 0.25f);
		const float perPodVolume = performance.engineVolume / static_cast<float>(count);
		for (std::size_t index = 0; index < count; ++index) {
			EngineSettings runtime = settings.engines[index % settings.engines.size()];
			if (settings.design.propulsionPlacement == PlacementMode::Auto) {
				runtime.length = std::clamp(
					settings.dimensions.length * 0.24f + std::sqrt(perPodVolume) * 0.18f,
					0.72f,
					settings.dimensions.length * 0.42f
				);
				runtime.radius = std::max(
					0.12f,
					std::sqrt(perPodVolume / (PI * runtime.length * 1.65f))
				);
				runtime.center = engineCenter(
					settings, layout, index, count, runtime.radius
				);
				runtime.nozzleDepth = std::max(runtime.radius * 0.62f, 0.12f);
			}
			const float podCapacity = PI * runtime.radius * runtime.radius
				* runtime.length * technology * 4.8f;
			const float requiredPodThrust = performance.requiredThrust / static_cast<float>(count);
			const int cells = std::clamp(
				static_cast<int>(std::ceil(requiredPodThrust / std::max(podCapacity, EPSILON))),
				1,
				6
			);
			result.push_back({
				runtime,
				{0.0f, 0.0f, -1.0f},
				runtime.radius * 0.72f,
				podCapacity * static_cast<float>(cells),
				cells
			});
		}
		return result;
	}

	std::vector<gen_model::spaceship::design::EnvelopeStation> stations(
		const Settings& settings,
		const std::vector<gen_model::spaceship::design::ModuleVolume>& modules
	) {
		float halfWidth = settings.hull.width * 0.5f;
		float top = settings.hull.height * 0.5f + settings.hull.crown;
		float bottom = -settings.hull.height * 0.5f - settings.hull.keel;
		for (const auto& item : modules) {
			if (!item.protectedByEnvelope)
				continue;
			halfWidth = std::max(halfWidth, std::abs(item.center.x) + item.halfExtents.x + settings.design.moduleClearance);
			top = std::max(top, item.center.y + item.halfExtents.y + settings.design.moduleClearance);
			bottom = std::min(bottom, item.center.y - item.halfExtents.y - settings.design.moduleClearance);
		}
		const float halfLength = std::max(settings.hull.length * 0.5f, settings.dimensions.length * 0.38f);
		const std::array<float, 9> positions{
			halfLength, halfLength * 0.82f, halfLength * 0.58f, halfLength * 0.30f,
			0.0f, -halfLength * 0.34f, -halfLength * 0.62f, -halfLength * 0.84f, -halfLength
		};
		const std::array<float, 9> scales{
			0.05f, 0.34f, 0.70f, 0.92f, 1.0f, 0.96f, 0.88f, 0.72f, settings.hull.rearTaper
		};
	std::vector<gen_model::spaceship::design::EnvelopeStation> result;
		result.reserve(positions.size());
		for (std::size_t index = 0; index < positions.size(); ++index)
			result.push_back({
				positions[index],
				halfWidth * scales[index],
				top * (0.16f + scales[index] * 0.84f),
				bottom * (0.16f + scales[index] * 0.84f),
				halfWidth * scales[index] * 0.82f
			});
		return result;
	}

	std::vector<gen_model::spaceship::design::SurfaceSample> samples(
		const Settings& settings,
		const std::vector<gen_model::spaceship::design::EnvelopeStation>& hullStations,
		const std::vector<gen_model::spaceship::design::EnginePod>& pods
	) {
		std::vector<gen_model::spaceship::design::SurfaceSample> result;
		result.reserve(hullStations.size() * 4u + pods.size() * 2u);
		for (const auto& station : hullStations) {
			result.push_back({
				{station.halfWidth * 0.86f, station.top, station.z},
				{0.0f, 1.0f, 0.0f},
				{1.0f, 0.0f, 0.0f},
				station.top - station.bottom,
				0
			});
			result.push_back({
				{-station.halfWidth * 0.86f, station.top, station.z},
				{0.0f, 1.0f, 0.0f},
				{-1.0f, 0.0f, 0.0f},
				station.top - station.bottom,
				0
			});
		}
		for (const auto& pod : pods) {
			result.push_back({
				{pod.runtime.center.x, pod.runtime.center.y + pod.runtime.radius, pod.runtime.center.z},
				{0.0f, 1.0f, 0.0f},
				{1.0f, 0.0f, 0.0f},
				pod.runtime.radius * 2.0f,
				1
			});
		}
		(void)settings;
		return result;
	}

	Point3 weightedCenter(
		const std::vector<gen_model::spaceship::design::ModuleVolume>& modules
	) {
		Point3 sum{};
		float weight = 0.0f;
		for (const auto& item : modules) {
			sum = sum + item.center * item.mass;
			weight += item.mass;
		}
		return weight <= EPSILON ? Point3{} : sum * (1.0f / weight);
	}

	void alignThrustAxis(gen_model::spaceship::design::PreliminaryDesign& design) {
		for (int iteration = 0; iteration < 6; ++iteration) {
			const Point3 massCenter = weightedCenter(design.coreModules);
			Point3 thrustCenter{};
			float thrust = 0.0f;
			for (const auto& pod : design.enginePods) {
				thrustCenter = thrustCenter + pod.runtime.center * pod.thrustCapacity;
				thrust += pod.thrustCapacity;
			}
			if (thrust <= EPSILON)
				return;
			thrustCenter = thrustCenter * (1.0f / thrust);
			const float correction = massCenter.y - thrustCenter.y;
			if (std::abs(correction) <= 0.0001f)
				return;
			for (auto& pod : design.enginePods)
				pod.runtime.center.y += correction;
			for (auto& module : design.coreModules)
				if (module.kind == ModuleKind::EngineCore || module.kind == ModuleKind::FuelTank)
					module.center.y += correction;
		}
	}

	void connectCoreGraph(
		gen_model::spaceship::design::PreliminaryDesign& result
	) {
		if (result.nodes.size() < 2u)
			return;
		for (std::size_t index = 1; index < result.nodes.size(); ++index)
			result.links.push_back({0u, index, std::max(0.12f, result.nodes[index].load * 0.08f)});
	}

	bool boxOverlap(
		Point3 leftCenter,
		Point3 leftExtents,
		Point3 rightCenter,
		Point3 rightExtents,
		float clearance
	) {
		return std::abs(leftCenter.x - rightCenter.x)
			<= leftExtents.x + rightExtents.x + clearance
			&& std::abs(leftCenter.y - rightCenter.y)
			<= leftExtents.y + rightExtents.y + clearance
			&& std::abs(leftCenter.z - rightCenter.z)
			<= leftExtents.z + rightExtents.z + clearance;
	}

	bool segmentIntersectsBox(
		Point3 start,
		Point3 end,
		Point3 boxCenter,
		Point3 boxExtents,
		float clearance
	) {
		const Point3 delta = end - start;
		const std::array<float, 3> starts{start.x, start.y, start.z};
		const std::array<float, 3> deltas{delta.x, delta.y, delta.z};
		const std::array<float, 3> centers{boxCenter.x, boxCenter.y, boxCenter.z};
		const std::array<float, 3> extents{boxExtents.x, boxExtents.y, boxExtents.z};
		float minimum = 0.0f;
		float maximum = 1.0f;
		for (std::size_t axis = 0; axis < starts.size(); ++axis) {
			const float lower = centers[axis] - extents[axis] - clearance;
			const float upper = centers[axis] + extents[axis] + clearance;
			if (std::abs(deltas[axis]) <= EPSILON) {
				if (starts[axis] < lower || starts[axis] > upper)
					return false;
				continue;
			}
			const float inverse = 1.0f / deltas[axis];
			float near = (lower - starts[axis]) * inverse;
			float far = (upper - starts[axis]) * inverse;
			if (near > far)
				std::swap(near, far);
			minimum = std::max(minimum, near);
			maximum = std::min(maximum, far);
			if (minimum > maximum)
				return false;
		}
		return true;
	}

	Point3 safeDirection(Point3 direction) {
		const float magnitude = std::sqrt(gen_model::gen_types::dot(direction, direction));
		return magnitude <= EPSILON ? Point3{} : direction * (1.0f / magnitude);
	}

	struct PackageBounds {
		Point3 minimum{};
		Point3 maximum{};
	};

	PackageBounds packageBounds(
		const std::vector<gen_model::spaceship::design::ModuleVolume>& modules
	) {
		PackageBounds result{
			{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()},
			{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
				std::numeric_limits<float>::lowest()}
		};
		bool found = false;
		for (const auto& item : modules) {
			if (!item.protectedByEnvelope)
				continue;
			found = true;
			result.minimum.x = std::min(result.minimum.x, item.center.x - item.halfExtents.x);
			result.minimum.y = std::min(result.minimum.y, item.center.y - item.halfExtents.y);
			result.minimum.z = std::min(result.minimum.z, item.center.z - item.halfExtents.z);
			result.maximum.x = std::max(result.maximum.x, item.center.x + item.halfExtents.x);
			result.maximum.y = std::max(result.maximum.y, item.center.y + item.halfExtents.y);
			result.maximum.z = std::max(result.maximum.z, item.center.z + item.halfExtents.z);
		}
		if (!found)
			return {};
		return result;
	}

	bool needsMaintenanceAccess(
		const gen_model::spaceship::design::ModuleVolume& item
	) {
		return item.protectedByEnvelope && item.kind != ModuleKind::Cockpit
			&& item.kind != ModuleKind::MountFoundation;
	}

	bool ignoresMaintenancePair(
		const gen_model::spaceship::design::ModuleVolume& item,
		const gen_model::spaceship::design::ModuleVolume& other
	) {
		if (other.kind == ModuleKind::MountFoundation)
			return true;
		if (item.ownerIndex >= 0 && item.ownerIndex == other.ownerIndex)
			return true;
		return (item.kind == ModuleKind::Reactor && other.kind == ModuleKind::ServiceBay)
			|| (item.kind == ModuleKind::ServiceBay && other.kind == ModuleKind::Reactor);
	}

	struct AccessPath {
		Point3 start{};
		Point3 end{};
		float length = 0.0f;
	};

	AccessPath maintenancePath(
		const gen_model::spaceship::design::ModuleVolume& item,
		Point3 direction,
		const PackageBounds& bounds,
		float clearance
	) {
		direction = safeDirection(direction);
		const float projectedExtent = std::abs(direction.x) * item.halfExtents.x
			+ std::abs(direction.y) * item.halfExtents.y
			+ std::abs(direction.z) * item.halfExtents.z;
		const Point3 boundsCenter = (bounds.minimum + bounds.maximum) * 0.5f;
		const Point3 boundsExtents = (bounds.maximum - bounds.minimum) * 0.5f;
		const float farProjection = gen_model::gen_types::dot(boundsCenter - item.center, direction)
			+ std::abs(direction.x) * boundsExtents.x
			+ std::abs(direction.y) * boundsExtents.y
			+ std::abs(direction.z) * boundsExtents.z;
		const float exitDistance = std::max(
			projectedExtent + clearance,
			farProjection + clearance * 2.0f
		);
		return {
			item.center + direction * (projectedExtent + clearance * 0.25f),
			item.center + direction * exitDistance,
			exitDistance - projectedExtent
		};
	}

	std::size_t maintenanceBlocker(
		std::size_t index,
		const std::vector<gen_model::spaceship::design::ModuleVolume>& modules,
		const AccessPath& path,
		float clearance
	) {
		const auto& item = modules[index];
		for (std::size_t otherIndex = 0; otherIndex < modules.size(); ++otherIndex) {
			if (otherIndex == index || !modules[otherIndex].protectedByEnvelope)
				continue;
			const auto& other = modules[otherIndex];
			if (ignoresMaintenancePair(item, other))
				continue;
			if (segmentIntersectsBox(
				path.start, path.end, other.center, other.halfExtents, clearance
			))
				return otherIndex;
		}
		return modules.size();
	}

	void resolveMaintenanceAccess(
		std::vector<gen_model::spaceship::design::ModuleVolume>& modules,
		const Settings& settings
	) {
		const PackageBounds bounds = packageBounds(modules);
		const std::array<Point3, 6> cardinalDirections{
			Point3{1.0f, 0.0f, 0.0f}, Point3{-1.0f, 0.0f, 0.0f},
			Point3{0.0f, 1.0f, 0.0f}, Point3{0.0f, -1.0f, 0.0f},
			Point3{0.0f, 0.0f, 1.0f}, Point3{0.0f, 0.0f, -1.0f}
		};
		for (std::size_t index = 0; index < modules.size(); ++index) {
			auto& item = modules[index];
			if (!needsMaintenanceAccess(item))
				continue;

			struct Candidate {
				Point3 direction{};
				AccessPath path{};
				bool preferred = false;
			};
			std::vector<Candidate> candidates;
			candidates.reserve(cardinalDirections.size() + 1u);
			const Point3 preferred = safeDirection(item.accessDirection);
			auto appendCandidate = [&](Point3 direction, bool isPreferred) {
				direction = safeDirection(direction);
				if (gen_model::gen_types::dot(direction, direction) <= EPSILON)
					return;
				for (const auto& existing : candidates)
					if (gen_model::gen_types::dot(existing.direction, direction) > 1.0f - EPSILON)
						return;
				const Point3 heatDirection = safeDirection(item.heatDirection);
				if (item.kind == ModuleKind::EngineCore
					&& gen_model::gen_types::dot(direction, heatDirection) > 0.5f)
					return;
				candidates.push_back({
					direction,
					maintenancePath(item, direction, bounds, settings.design.moduleClearance),
					isPreferred
				});
			};
			appendCandidate(preferred, true);
			for (const Point3 direction : cardinalDirections)
				appendCandidate(direction, false);
			std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
				if (left.preferred != right.preferred)
					return left.preferred;
				return left.path.length < right.path.length;
			});
			for (const auto& candidate : candidates) {
				if (maintenanceBlocker(
					index, modules, candidate.path, settings.design.moduleClearance
				) != modules.size())
					continue;
				item.accessDirection = candidate.direction;
				break;
			}
		}
	}

	std::string diagnosticsText(const gen_model::spaceship::design::Audit& audit) {
		std::ostringstream result;
		for (const auto& diagnostic : audit.diagnostics)
			result << diagnostic << ';';
		return result.str();
	}
}

bool gen_model::spaceship::design::shapeAllowedFor(
	ModuleKind kind,
	ModuleShape shape
) noexcept {
	switch (kind) {
		case ModuleKind::Cockpit:
			return shape == ModuleShape::FacetedCapsule || shape == ModuleShape::ArmoredWedge;
		case ModuleKind::FuelTank:
			return shape == ModuleShape::Ellipsoid || shape == ModuleShape::CappedCylinder;
		case ModuleKind::Reactor:
			return shape == ModuleShape::ShieldedSphere || shape == ModuleShape::CappedCylinder;
		case ModuleKind::Magazine:
			return shape == ModuleShape::ChamferedBox || shape == ModuleShape::TaperedBox
				|| shape == ModuleShape::RadialWedge;
		case ModuleKind::EngineCore:
			return shape == ModuleShape::AxialFrustum || shape == ModuleShape::CappedCylinder;
		case ModuleKind::MountFoundation:
			return shape == ModuleShape::TaperedBeam || shape == ModuleShape::ChamferedBox;
		default:
			return shape == ModuleShape::ChamferedBox || shape == ModuleShape::TaperedBox;
	}
}

const char* gen_model::spaceship::design::layoutName(PropulsionLayout layout) noexcept {
	switch (layout) {
		case PropulsionLayout::Auto: return "auto";
		case PropulsionLayout::CentralCluster: return "central_cluster";
		case PropulsionLayout::SpineCluster: return "spine_cluster";
		case PropulsionLayout::TwinBoom: return "twin_boom";
		case PropulsionLayout::WingNacelles: return "wing_nacelles";
		case PropulsionLayout::DistributedAft: return "distributed_aft";
		case PropulsionLayout::CapitalSideBlocks: return "capital_side_blocks";
	}
	return "unknown";
}

gen_model::spaceship::design::PreliminaryDesign
gen_model::spaceship::design::planCore(const Settings& settings) {
	PreliminaryDesign result;
	const float burden = weaponBurden(settings);
	const float reactorBase = sphereVolume(settings.layout.reactorRadius);
	const Performance performance = solvePerformance(settings, burden, reactorBase);
	const PropulsionLayout layout = chooseLayout(settings, burden);
	result.metrics.selectedLayout = layout;
	result.metrics.massProxy = performance.mass;
	result.metrics.weaponBurden = burden;
	result.metrics.fuelVolume = performance.fuelVolume;
	result.metrics.engineVolume = performance.engineVolume;
	result.metrics.reactorVolume = performance.reactorVolume;
	result.metrics.requiredThrust = performance.requiredThrust;

	const auto cockpitShape = defaultShape(ModuleKind::Cockpit, settings.seed);
	result.coreModules.push_back(module(
		ModuleKind::Cockpit,
		cockpitShape,
		settings.cockpit.center,
		settings.cockpit.size * 0.5f + Point3{
			settings.design.moduleClearance,
			settings.design.moduleClearance,
			settings.design.moduleClearance
		},
		settings.cockpit.size.x * settings.cockpit.size.y * settings.cockpit.size.z,
		settings.cockpit.size.x * settings.cockpit.size.y * settings.cockpit.size.z * 0.55f,
		-1
	));
	const Point3 reactorCenter{0.0f, -settings.hull.height * 0.05f, -settings.hull.length * 0.08f};
	const float reactorRadius = std::max(
		settings.layout.reactorRadius,
		std::cbrt(std::max(performance.reactorVolume, EPSILON) / (4.0f * PI / 3.0f))
	);
	result.coreModules.push_back(module(
		ModuleKind::Reactor,
		defaultShape(ModuleKind::Reactor, settings.seed + 1u),
		reactorCenter,
		{reactorRadius, reactorRadius * 0.86f, reactorRadius},
		sphereVolume(reactorRadius),
		sphereVolume(reactorRadius) * 1.8f,
		-1
	));
	result.coreModules.push_back(module(
		ModuleKind::ServiceBay,
		ModuleShape::ChamferedBox,
		{0.0f, 0.0f, settings.hull.length * 0.02f},
		{
			std::max(settings.layout.primarySpineWidth, settings.hull.width * 0.22f),
			settings.hull.height * 0.30f,
			settings.layout.serviceBayLength * 0.5f
		},
		settings.layout.primarySpineWidth * settings.hull.height * settings.layout.serviceBayLength,
		settings.layout.serviceBayLength,
		-1
	));

	const std::size_t radiatorPairs = radiatorPairCount(settings, reactorRadius);
	const float radiatorHalfWidth = std::max(
		settings.armorDepth * 1.25f,
		settings.layout.radiatorScale * 0.14f
	);
	const float radiatorHalfLength = std::max(
		settings.layout.serviceBayLength * 0.18f,
		settings.layout.reactorRadius * 0.34f
	);
	const float radiatorHalfHeight = std::max(settings.armorDepth * 0.25f, 0.025f);
	const bool compactAirframe = layout != PropulsionLayout::DistributedAft
		&& layout != PropulsionLayout::CapitalSideBlocks;
	const float radiatorBaseX = std::max(
		settings.hull.width * 0.28f,
		reactorRadius + radiatorHalfWidth + settings.design.moduleClearance * 0.35f
	);
	const float radiatorY = compactAirframe
		? settings.hull.height * 0.410f + settings.hull.crown
			+ radiatorHalfHeight * 0.35f
		: settings.hull.height * 0.50f + settings.hull.crown + radiatorHalfHeight * 0.35f;
	for (std::size_t pair = 0; pair < radiatorPairs; ++pair) {
		const float rank = static_cast<float>(pair);
		const float x = radiatorBaseX
			+ rank * (radiatorHalfWidth * 2.25f + settings.design.moduleClearance * 0.55f);
		const float z = reactorCenter.z - settings.hull.length
			* (compactAirframe ? 0.015f : 0.15f)
			+ (rank - static_cast<float>(radiatorPairs - 1u) * 0.5f)
				* radiatorHalfLength * 0.62f;
		for (const float side : {-1.0f, 1.0f}) {
			const Point3 extents{
				radiatorHalfWidth,
				radiatorHalfHeight,
				radiatorHalfLength
			};
			auto radiator = module(
				ModuleKind::Radiator,
				ModuleShape::TaperedBox,
				{
					side * x,
					radiatorY,
					z
				},
				extents,
				boxVolume(extents) * 0.72f,
				8.0f * extents.x * extents.z * 0.008f,
				static_cast<int>(pair),
				false
			);
			radiator.accessDirection = {0.0f, 1.0f, 0.0f};
			radiator.heatDirection = {0.0f, 1.0f, 0.0f};
			result.coreModules.push_back(radiator);
		}
	}

	result.enginePods = resolveEngines(settings, layout, performance);
	const float fuelPerPod = result.enginePods.empty()
		? 0.0f
		: performance.fuelVolume / static_cast<float>(result.enginePods.size());
	for (std::size_t index = 0; index < result.enginePods.size(); ++index) {
		const auto& pod = result.enginePods[index];
		result.coreModules.push_back(module(
			ModuleKind::EngineCore,
			defaultShape(ModuleKind::EngineCore, settings.seed + static_cast<std::uint32_t>(index)),
			pod.runtime.center + Point3{0.0f, 0.0f, pod.runtime.length * 0.10f},
			{pod.runtime.radius * 1.12f, pod.runtime.radius * 0.92f, pod.runtime.length * 0.46f},
			PI * pod.runtime.radius * pod.runtime.radius * pod.runtime.length * 1.5f,
			pod.thrustCapacity * 0.12f,
			static_cast<int>(index)
		));
		const float tankRadius = std::cbrt(std::max(fuelPerPod, EPSILON) / (4.0f * PI / 3.0f));
		Point3 fuelCenter = pod.runtime.center
			+ Point3{0.0f, 0.0f, pod.runtime.length * 0.60f};
		if (layout == PropulsionLayout::CentralCluster) {
			const float side = fuelCenter.x < 0.0f ? -1.0f : 1.0f;
			fuelCenter.x += side * std::max(tankRadius * 0.95f, pod.runtime.radius * 0.85f);
		}
		result.coreModules.push_back(module(
			ModuleKind::FuelTank,
			defaultShape(ModuleKind::FuelTank, settings.seed + 101u + static_cast<std::uint32_t>(index)),
			fuelCenter,
			{std::max(tankRadius, pod.runtime.radius * 0.80f),
				std::max(tankRadius * 0.72f, pod.runtime.radius * 0.65f),
				std::max(tankRadius * 1.15f, pod.runtime.length * 0.32f)},
			fuelPerPod,
			fuelPerPod * 0.08f,
			static_cast<int>(index)
		));
	}
	for (auto& item : result.coreModules) {
		const bool layeredCapitalMachinery = layout == PropulsionLayout::DistributedAft
			|| layout == PropulsionLayout::CapitalSideBlocks;
		switch (item.kind) {
			case ModuleKind::EngineCore:
				item.heatDirection = {0.0f, 0.0f, -1.0f};
				item.accessDirection = layeredCapitalMachinery
					? Point3{0.0f, item.center.y < 0.0f ? -1.0f : 1.0f, 0.0f}
					: (std::abs(item.center.x) > EPSILON
					? Point3{std::copysign(1.0f, item.center.x), 0.0f, 0.0f}
					: Point3{0.0f, 1.0f, 0.0f});
				break;
			case ModuleKind::FuelTank:
				item.accessDirection = layeredCapitalMachinery
					? Point3{0.0f, item.center.y < 0.0f ? -1.0f : 1.0f, 0.0f}
					: Point3{0.0f, 1.0f, 0.0f};
				break;
			case ModuleKind::Reactor:
				item.accessDirection = {0.0f, -1.0f, 0.0f};
				break;
			case ModuleKind::ServiceBay:
				item.accessDirection = {0.0f, 1.0f, 0.0f};
				break;
			default:
				break;
		}
	}
	alignThrustAxis(result);
	resolveMaintenanceAccess(result.coreModules, settings);

	for (std::size_t index = 0; index < result.coreModules.size(); ++index) {
		const auto& item = result.coreModules[index];
		result.nodes.push_back({item.center, std::max(item.mass, 0.1f), static_cast<int>(index)});
	}
	connectCoreGraph(result);
	result.centralStations = stations(settings, result.coreModules);
	result.candidateSurfaces = samples(settings, result.centralStations, result.enginePods);
	result.metrics.availableThrust = 0.0f;
	for (const auto& pod : result.enginePods)
		result.metrics.availableThrust += pod.thrustCapacity;
	result.metrics.massCenter = weightedCenter(result.coreModules);
	Point3 thrustCenter{};
	for (const auto& pod : result.enginePods)
		thrustCenter = thrustCenter + pod.runtime.center * pod.thrustCapacity;
	if (result.metrics.availableThrust > EPSILON)
		result.metrics.thrustCenter = thrustCenter * (1.0f / result.metrics.availableThrust);
	return result;
}

gen_model::spaceship::design::DesignPlan gen_model::spaceship::design::complete(
	const Settings& settings,
	const PreliminaryDesign& preliminary,
	const std::vector<ModuleVolume>& weaponModules,
	const std::vector<MountSettings>& resolvedMounts
) {
	DesignPlan result;
	static_cast<PreliminaryDesign&>(result) = preliminary;
	result.resolvedMounts = resolvedMounts;
	result.weaponModules.reserve(weaponModules.size());
	for (std::size_t index = 0; index < weaponModules.size(); ++index) {
		ModuleVolume module = weaponModules[index];
		if (!fitShapeVolume(module.shape, module.halfExtents, module.requiredVolume)) {
			std::ostringstream message;
			message << "weapon module " << index << " cannot be packaged in its shape";
			throw std::invalid_argument(message.str());
		}
		module.actualVolume = shapeVolume(module.shape, module.halfExtents);
		if (!std::isfinite(module.actualVolume)) {
			std::ostringstream message;
			message << "weapon module " << index << " has a non-finite volume";
			throw std::invalid_argument(message.str());
		}
		result.weaponModules.push_back(module);
	}
	result.coreModules.insert(
		result.coreModules.end(), result.weaponModules.begin(), result.weaponModules.end()
	);
	resolveMaintenanceAccess(result.coreModules, settings);
	for (std::size_t index = 0; index < result.weaponModules.size(); ++index)
		result.weaponModules[index].accessDirection =
			result.coreModules[result.coreModules.size() - result.weaponModules.size() + index].accessDirection;

	// A preliminary graph normally already has one node per core module.  Direct
	// planner callers may provide a synthetic preliminary design without nodes;
	// materialise the missing core nodes before attaching weapon loads.
	if (result.nodes.empty() && !result.coreModules.empty()) {
		result.nodes.reserve(result.coreModules.size());
		for (std::size_t index = 0; index < result.coreModules.size() - result.weaponModules.size(); ++index) {
			const auto& item = result.coreModules[index];
			result.nodes.push_back({item.center, std::max(item.mass, 0.1f), static_cast<int>(index)});
		}
		connectCoreGraph(result);
	}

	const std::size_t weaponModuleStart = result.coreModules.size() - result.weaponModules.size();
	const std::size_t weaponNodeStart = result.nodes.size();
	result.nodes.reserve(result.nodes.size() + result.weaponModules.size());
	for (std::size_t index = 0; index < result.weaponModules.size(); ++index) {
		const auto& item = result.weaponModules[index];
		result.nodes.push_back({
			item.center,
			std::max(item.mass, 0.1f),
			static_cast<int>(weaponModuleStart + index)
		});
	}
	for (std::size_t index = 0; index < result.weaponModules.size(); ++index) {
		const std::size_t nodeIndex = weaponNodeStart + index;
		if (nodeIndex == 0u)
			continue;
		std::size_t anchor = 0u;
		float bestDistance = std::numeric_limits<float>::max();
		const auto& weapon = result.weaponModules[index];
		// Prefer the engine core for the owning mount.  This gives every magazine
		// and foundation an explicit load path into the propulsion/primary graph.
		for (std::size_t candidate = 0; candidate < nodeIndex; ++candidate) {
			const auto& node = result.nodes[candidate];
			if (node.moduleIndex < 0
				|| static_cast<std::size_t>(node.moduleIndex) >= result.coreModules.size())
				continue;
			const auto& owner = result.coreModules[static_cast<std::size_t>(node.moduleIndex)];
			const bool sameOwnerEngine = owner.kind == ModuleKind::EngineCore
				&& owner.ownerIndex >= 0 && owner.ownerIndex == weapon.ownerIndex;
			const float candidateDistance = sameOwnerEngine
				? 0.0f
				: gen_model::gen_types::dot(node.position - weapon.center, node.position - weapon.center);
			if (candidateDistance < bestDistance) {
				bestDistance = candidateDistance;
				anchor = candidate;
			}
		}
		result.links.push_back({
			anchor,
			nodeIndex,
			std::max(0.12f, result.nodes[nodeIndex].load * 0.08f)
		});
	}

	result.metrics.magazineVolume = 0.0f;
	for (const auto& item : result.weaponModules) {
		if (item.kind == ModuleKind::Magazine)
			result.metrics.magazineVolume += item.actualVolume;
	}
	result.metrics.massProxy = 0.0f;
	for (const auto& item : result.coreModules)
		result.metrics.massProxy += std::max(item.mass, 0.0f);
	const float technology = std::max(settings.design.engineTechnology, 0.25f);
	const float acceleration = std::max(settings.design.targetAcceleration, 0.05f);
	result.metrics.requiredThrust = result.metrics.massProxy * acceleration;
	result.metrics.engineVolume = result.metrics.requiredThrust / (technology * 4.8f);
	result.metrics.fuelVolume = result.metrics.requiredThrust
		* std::max(settings.design.endurance, 0.05f) / (technology * 3.6f);
	const float reactorBase = sphereVolume(settings.layout.reactorRadius);
	result.metrics.reactorVolume = std::max(
		result.metrics.requiredThrust * 0.12f,
		weaponPower(settings)
	) / 2.8f + reactorBase * 0.42f;
	result.metrics.availableThrust = 0.0f;
	Point3 thrustCenter{};
	for (const auto& pod : result.enginePods) {
		result.metrics.availableThrust += std::max(pod.thrustCapacity, 0.0f);
		thrustCenter = thrustCenter + pod.runtime.center * std::max(pod.thrustCapacity, 0.0f);
	}
	result.metrics.massCenter = weightedCenter(result.coreModules);
	result.metrics.thrustCenter = result.metrics.availableThrust <= EPSILON
		? Point3{}
		: thrustCenter * (1.0f / result.metrics.availableThrust);
	// External magazines/foundations receive local fairings; they must not
	// inflate the central pressure-hull stations.
	result.centralStations = stations(settings, preliminary.coreModules);
	return result;
}

gen_model::spaceship::design::Audit gen_model::spaceship::design::audit(
	const Settings& settings,
	const DesignPlan& plan
) {
	Audit result;
	for (std::size_t index = 0; index < plan.coreModules.size(); ++index) {
		const auto& item = plan.coreModules[index];
		if (!shapeAllowedFor(item.kind, item.shape)) {
			++result.invalidShapeVolumes;
			std::ostringstream message;
			message << "invalid component shape module=" << index;
			result.diagnostics.push_back(message.str());
		}
		const float representedVolume = shapeVolume(item.shape, item.halfExtents);
		const float volumeTolerance = std::max(EPSILON, std::abs(representedVolume) * 0.0001f);
		if (!finitePoint(item.center) || !finitePoint(item.halfExtents)
			|| item.halfExtents.x <= EPSILON || item.halfExtents.y <= EPSILON
			|| item.halfExtents.z <= EPSILON || !std::isfinite(item.actualVolume)
			|| std::abs(item.actualVolume - representedVolume) > volumeTolerance) {
			++result.invalidShapeVolumes;
			std::ostringstream message;
			message << "module volume does not match shape extents module=" << index;
			result.diagnostics.push_back(message.str());
		}
		if (item.actualVolume + EPSILON < item.requiredVolume) {
			++result.uncontainedModules;
			std::ostringstream message;
			message << "module volume is under-packaged module=" << index;
			result.diagnostics.push_back(message.str());
		}
	}

	// Validate the full graph rather than inferring connectivity from an edge
	// count.  Weapon modules are required to have a node and at least one load
	// path; malformed direct planner inputs therefore fail deterministically.
	if (!plan.nodes.empty()) {
		std::vector<std::vector<std::size_t>> adjacency(plan.nodes.size());
		std::vector<std::size_t> degree(plan.nodes.size(), 0u);
		for (const auto& link : plan.links) {
			if (link.startNode >= plan.nodes.size() || link.endNode >= plan.nodes.size()) {
				++result.disconnectedNodes;
				result.diagnostics.push_back("structural link references an invalid node");
				continue;
			}
			if (link.startNode == link.endNode || !std::isfinite(link.thickness)
				|| link.thickness <= EPSILON) {
				++result.disconnectedNodes;
				result.diagnostics.push_back("structural link is degenerate");
				continue;
			}
			adjacency[link.startNode].push_back(link.endNode);
			adjacency[link.endNode].push_back(link.startNode);
			++degree[link.startNode];
			++degree[link.endNode];
		}
		std::vector<bool> visited(plan.nodes.size(), false);
		std::queue<std::size_t> pending;
		visited[0] = true;
		pending.push(0u);
		while (!pending.empty()) {
			const std::size_t node = pending.front();
			pending.pop();
			for (const std::size_t neighbour : adjacency[node]) {
				if (visited[neighbour])
					continue;
				visited[neighbour] = true;
				pending.push(neighbour);
			}
		}
		for (std::size_t index = 0; index < visited.size(); ++index) {
			if (visited[index])
				continue;
			++result.disconnectedNodes;
			std::ostringstream message;
			message << "structural node is unreachable node=" << index;
			result.diagnostics.push_back(message.str());
		}
		std::vector<int> moduleNodes(plan.coreModules.size(), -1);
		for (std::size_t index = 0; index < plan.nodes.size(); ++index) {
			const int moduleIndex = plan.nodes[index].moduleIndex;
			if (moduleIndex < 0 || static_cast<std::size_t>(moduleIndex) >= plan.coreModules.size()) {
				++result.disconnectedNodes;
				result.diagnostics.push_back("structural node has no valid module owner");
				continue;
			}
			moduleNodes[static_cast<std::size_t>(moduleIndex)] = static_cast<int>(index);
		}
		for (std::size_t index = 0; index < plan.coreModules.size(); ++index) {
			const auto kind = plan.coreModules[index].kind;
			if (kind != ModuleKind::Magazine && kind != ModuleKind::MountFoundation)
				continue;
			if (moduleNodes[index] < 0 || degree[static_cast<std::size_t>(moduleNodes[index])] == 0u) {
				++result.disconnectedNodes;
				std::ostringstream message;
				message << "weapon module has no structural load path module=" << index;
				result.diagnostics.push_back(message.str());
			}
		}
	} else if (!plan.coreModules.empty()) {
		result.disconnectedNodes = plan.coreModules.size();
		result.diagnostics.push_back("structural graph has no nodes");
	}

	if (plan.metrics.availableThrust + EPSILON < plan.metrics.requiredThrust) {
		++result.propulsionViolations;
		std::ostringstream message;
		message << "required=" << plan.metrics.requiredThrust
			<< " available=" << plan.metrics.availableThrust;
		result.diagnostics.push_back(message.str());
	}
	const float centroidXTolerance = std::max(settings.dimensions.width * 0.02f, 0.02f);
	const float centroidYTolerance = std::max(settings.dimensions.height * 0.02f, 0.02f);
	if (std::abs(plan.metrics.massCenter.x - plan.metrics.thrustCenter.x) > centroidXTolerance
		|| std::abs(plan.metrics.massCenter.y - plan.metrics.thrustCenter.y) > centroidYTolerance) {
		++result.centroidViolations;
		std::ostringstream message;
		message << "mass or thrust centroid is unbalanced mass=("
			<< plan.metrics.massCenter.x << ',' << plan.metrics.massCenter.y
			<< ") thrust=(" << plan.metrics.thrustCenter.x << ','
			<< plan.metrics.thrustCenter.y << ") tolerance=("
			<< centroidXTolerance << ',' << centroidYTolerance << ')';
		result.diagnostics.push_back(message.str());
	}

	for (std::size_t index = 0; index < plan.coreModules.size(); ++index) {
		const auto& item = plan.coreModules[index];
		if (item.kind != ModuleKind::EngineCore)
			continue;
		const Point3 heatDirection = safeDirection(item.heatDirection);
		if (gen_model::gen_types::dot(heatDirection, heatDirection) <= EPSILON)
			continue;
		const float heatReach = std::max({item.halfExtents.x, item.halfExtents.y, item.halfExtents.z})
			* 1.5f + settings.design.moduleClearance;
		const float projectedExtent = std::abs(heatDirection.x) * item.halfExtents.x
			+ std::abs(heatDirection.y) * item.halfExtents.y
			+ std::abs(heatDirection.z) * item.halfExtents.z;
		const Point3 heatStart = item.center + heatDirection * projectedExtent;
		const Point3 heatCenter = heatStart + heatDirection * (heatReach * 0.50f);
		const Point3 heatExtents{
			item.halfExtents.x * 0.72f + std::abs(heatDirection.x) * heatReach * 0.50f,
			item.halfExtents.y * 0.72f + std::abs(heatDirection.y) * heatReach * 0.50f,
			item.halfExtents.z * 0.72f + std::abs(heatDirection.z) * heatReach * 0.50f
		};
		for (std::size_t otherIndex = 0; otherIndex < plan.coreModules.size(); ++otherIndex) {
			const auto& other = plan.coreModules[otherIndex];
			if (otherIndex == index || !other.protectedByEnvelope
				|| (other.kind != ModuleKind::Cockpit && other.kind != ModuleKind::Magazine
					&& other.kind != ModuleKind::Reactor))
				continue;
			if (boxOverlap(heatCenter, heatExtents, other.center, other.halfExtents, settings.design.moduleClearance)) {
				++result.thermalViolations;
				std::ostringstream message;
				message << "engine heat zone overlaps module=" << otherIndex
					<< " owner=" << other.ownerIndex;
				result.diagnostics.push_back(message.str());
			}
		}
	}

	const PackageBounds maintenanceBounds = packageBounds(plan.coreModules);
	for (std::size_t index = 0; index < plan.coreModules.size(); ++index) {
		const auto& item = plan.coreModules[index];
		if (!needsMaintenanceAccess(item))
			continue;
		const Point3 accessDirection = safeDirection(item.accessDirection);
		if (gen_model::gen_types::dot(accessDirection, accessDirection) <= EPSILON) {
			++result.accessViolations;
			result.diagnostics.push_back("protected module has no maintenance access direction");
			continue;
		}
		const AccessPath path = maintenancePath(
			item, accessDirection, maintenanceBounds, settings.design.moduleClearance
		);
		const std::size_t blocker = maintenanceBlocker(
			index, plan.coreModules, path, settings.design.moduleClearance
		);
		if (blocker == plan.coreModules.size())
			continue;
		++result.accessViolations;
		std::ostringstream message;
		message << "maintenance access blocked module=" << index
			<< " by=" << blocker << " origin=(" << item.center.x << ','
			<< item.center.y << ',' << item.center.z << ") direction=("
			<< accessDirection.x << ',' << accessDirection.y << ','
			<< accessDirection.z << ") blocker=(" << plan.coreModules[blocker].center.x
			<< ',' << plan.coreModules[blocker].center.y << ','
			<< plan.coreModules[blocker].center.z << ')';
		result.diagnostics.push_back(message.str());
	}

	if (plan.enginePods.empty() && plan.metrics.requiredThrust > EPSILON) {
		++result.podCapacityViolations;
		++result.propulsionViolations;
		result.diagnostics.push_back("no engine pods provide required thrust capacity");
	}
	for (std::size_t index = 0; index < plan.enginePods.size(); ++index) {
		const auto& pod = plan.enginePods[index];
		const float requiredPodThrust = plan.metrics.requiredThrust
			/ static_cast<float>(plan.enginePods.size());
		if (!finitePoint(pod.runtime.center) || !std::isfinite(pod.runtime.radius)
			|| !std::isfinite(pod.runtime.length) || pod.runtime.radius <= EPSILON
			|| pod.runtime.length <= EPSILON || pod.nozzleCells < 1
			|| !std::isfinite(pod.thrustCapacity) || pod.thrustCapacity <= EPSILON
			|| pod.nozzleRadius <= EPSILON || pod.nozzleRadius > pod.runtime.radius + EPSILON) {
			++result.podFitViolations;
			result.diagnostics.push_back("engine pod has invalid dimensions or nozzle fit");
		}
		if (pod.thrustCapacity + EPSILON < requiredPodThrust) {
			++result.podCapacityViolations;
			++result.propulsionViolations;
			std::ostringstream message;
			message << "engine pod capacity is insufficient pod=" << index
				<< " required=" << requiredPodThrust
				<< " available=" << pod.thrustCapacity;
			result.diagnostics.push_back(message.str());
		}
		bool matchedCore = false;
		for (const auto& module : plan.coreModules) {
			if (module.kind != ModuleKind::EngineCore || module.ownerIndex != static_cast<int>(index))
				continue;
			matchedCore = module.halfExtents.x + EPSILON >= pod.runtime.radius * 0.75f
				&& module.halfExtents.y + EPSILON >= pod.runtime.radius * 0.60f
				&& module.halfExtents.z + EPSILON >= pod.runtime.length * 0.20f;
			break;
		}
		if (!matchedCore) {
			++result.podFitViolations;
			result.diagnostics.push_back("engine pod has no fitting engine-core volume");
		}
	}
	return result;
}

bool gen_model::spaceship::design::Audit::valid() const noexcept {
	return invalidShapeVolumes == 0u
		&& uncontainedModules == 0u
		&& disconnectedNodes == 0u
		&& thermalViolations == 0u
		&& accessViolations == 0u
		&& centroidViolations == 0u
		&& propulsionViolations == 0u
		&& podCapacityViolations == 0u
		&& podFitViolations == 0u;
}

void gen_model::spaceship::design::requireValid(
	const Settings& settings,
	const DesignPlan& plan
) {
	const Audit result = audit(settings, plan);
	if (!result.valid())
		throw std::invalid_argument("Spaceship design audit failed: " + diagnosticsText(result));
}
