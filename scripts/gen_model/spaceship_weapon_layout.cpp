#include "spaceship_weapon_layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
	constexpr float PI = 3.14159265358979323846f;
	constexpr float EPSILON = 0.00001f;

	using Point3 = gen_model::gen_types::Point3;
	using Settings = gen_model::spaceship::Settings;
	using MountSettings = gen_model::spaceship::MountSettings;
	using Layout = gen_model::spaceship::weapon_layout::CandidateLayout;
	using Module = gen_model::spaceship::design::ModuleVolume;
	using ModuleKind = gen_model::spaceship::design::ModuleKind;
	using ModuleShape = gen_model::spaceship::design::ModuleShape;
	using LayoutSymmetry = gen_model::spaceship::LayoutSymmetry;
	using PropulsionLayout = gen_model::spaceship::PropulsionLayout;
	using WeaponCoverage = gen_model::spaceship::WeaponCoverage;

	float length(Point3 point) {
		return std::sqrt(gen_model::gen_types::dot(point, point));
	}

	float distance(Point3 left, Point3 right) {
		return length(left - right);
	}

	float smoothStep(float amount) {
		amount = std::clamp(amount, 0.0f, 1.0f);
		return amount * amount * (3.0f - 2.0f * amount);
	}

	std::string pointText(Point3 point) {
		std::ostringstream result;
		result << '(' << point.x << ',' << point.y << ',' << point.z << ')';
		return result.str();
	}

	float pointSegmentDistance(Point3 point, Point3 start, Point3 end) {
		const Point3 axis = end - start;
		const float denominator = gen_model::gen_types::dot(axis, axis);
		if (denominator <= EPSILON)
			return distance(point, start);
		const float amount = std::clamp(
			gen_model::gen_types::dot(point - start, axis) / denominator,
			0.0f,
			1.0f
		);
		return distance(point, start + axis * amount);
	}

	float moduleVolume(ModuleShape shape, Point3 extents) {
		if (shape == ModuleShape::Ellipsoid || shape == ModuleShape::ShieldedSphere)
			return 4.0f * PI * extents.x * extents.y * extents.z / 3.0f;
		return 8.0f * extents.x * extents.y * extents.z;
	}

	Module makeModule(
		ModuleKind kind,
		ModuleShape shape,
		Point3 center,
		Point3 extents,
		float requiredVolume,
		int ownerIndex
	) {
		float actualVolume = moduleVolume(shape, extents);
		if (actualVolume + EPSILON < requiredVolume) {
			const float scale = std::cbrt(requiredVolume / std::max(actualVolume, EPSILON));
			extents = extents * scale;
			actualVolume = moduleVolume(shape, extents);
		}
		Module result;
		result.kind = kind;
		result.shape = shape;
		result.center = center;
		result.halfExtents = extents;
		result.requiredVolume = requiredVolume;
		result.actualVolume = actualVolume;
		result.chamfer = std::min({extents.x, extents.y, extents.z}) * 0.25f;
		result.mass = result.actualVolume * 0.16f;
		result.ownerIndex = ownerIndex;
		result.protectedByEnvelope = true;
		const float verticalSide = center.y < 0.0f ? -1.0f : 1.0f;
		result.accessDirection = {0.0f, verticalSide, 0.0f};
		return result;
	}

	bool denseBattery(const Settings& settings) {
		if (settings.design.weaponLayout.batteryStyle == gen_model::spaceship::BatteryStyle::External)
			return true;
		if (settings.design.weaponLayout.batteryStyle == gen_model::spaceship::BatteryStyle::Integrated)
			return false;
		return settings.mounts.size() >= 6u;
	}

	// A sparse carrier battery has room for a real wing/sponson under each deck.
	// Treating it like a dense siege rail forces the socket upward purely to clear
	// the traverse cone, which produces an exposed antenna-like pedestal.  The
	// envelope generator uses the same architecture policy to grow the supporting
	// sponson before the mount is seated on it.
	bool carrierSponsonBattery(const Settings& settings) {
		return settings.layout.archetype == "carrier"
			&& settings.design.weaponLayout.batteryStyle
			== gen_model::spaceship::BatteryStyle::External
			&& settings.mounts.size() <= 16u;
	}

	float architectureBias(PropulsionLayout layout) {
		switch (layout) {
			case PropulsionLayout::WingNacelles: return 0.10f;
			case PropulsionLayout::TwinBoom: return -0.08f;
			case PropulsionLayout::SpineCluster: return -0.04f;
			case PropulsionLayout::DistributedAft: return 0.08f;
			case PropulsionLayout::CapitalSideBlocks: return 0.12f;
			case PropulsionLayout::Auto:
			case PropulsionLayout::CentralCluster:
			default: return 0.0f;
		}
	}

	Point3 coverageDirection(
		WeaponCoverage coverage,
		float side,
		float verticalSide,
		std::size_t pairIndex
	) {
		if (coverage == WeaponCoverage::Forward)
			return {0.0f, 0.0f, 1.0f};
		if (coverage == WeaponCoverage::Broadside)
			return gen_model::gen_types::normalize(Point3{side, 0.0f, 0.32f});
		switch (pairIndex % 4u) {
			case 0u: return {0.0f, 0.0f, 1.0f};
			case 1u: return {side, 0.0f, 0.0f};
			case 2u: return {0.0f, 0.0f, -1.0f};
			default: return {0.0f, verticalSide, 0.0f};
		}
	}

	void moveOutsideUnitParentSphere(MountSettings& mount) {
		const float requiredDistance = 1.0f + mount.turretRadius + 0.02f;
		const float currentDistance = length(mount.position);
		if (currentDistance >= requiredDistance)
			return;
		const Point3 direction = currentDistance > EPSILON
			? mount.position * (1.0f / currentDistance)
			: Point3{0.0f, 0.0f, 1.0f};
		const float shift = requiredDistance - currentDistance;
		mount.position = mount.position + direction * shift;
		// Keep the structural root moving with the socket so this correction does
		// not create a longer connector or leave the support behind in the hull.
		mount.supportRoot = mount.supportRoot + direction * (shift * 0.65f);
	}

	MountSettings resolvedMount(
		const Settings& settings,
		std::size_t index,
		int variant,
		const gen_model::spaceship::design::PreliminaryDesign& preliminary
	) {
		const MountSettings& source = settings.mounts[index];
		MountSettings result = source;
		const std::size_t count = settings.mounts.size();
		const float radius = source.turretRadius;
		const bool dense = denseBattery(settings);
		const float sweepRadians = source.traverseHalfAngleDegrees * PI / 180.0f;
		float socketHeight = std::max(source.socketHeight, radius * 0.72f);
		if (dense && !carrierSponsonBattery(settings)) {
			// A long barrel needs enough pedestal height for the lower edge of its
			// traverse cone to clear the battery rail over the full barrel length.
			const float sweptDrop = std::sin(sweepRadians) * source.barrelLength;
			const float railHalfHeight = std::max(source.supportHeight, radius * 2.4f) * 0.38f;
			socketHeight = std::max(
				socketHeight,
				sweptDrop + source.barrelRadius + railHalfHeight - radius
			);
		}
		result.socketHeight = socketHeight;
		const bool paired = settings.design.weaponLayout.symmetry == LayoutSymmetry::Bilateral;
		const std::size_t pairIndex = paired ? index / 2u : index;
		const bool positiveSide = (index % 2u) == 0u;
		const float side = positiveSide ? 1.0f : -1.0f;
		const std::size_t pairCount = paired ? (count + 1u) / 2u : count;
		const float spread = pairCount <= 1u
			? 0.0f
			: static_cast<float>(pairIndex) / static_cast<float>(pairCount - 1u);
		if (settings.design.weaponLayout.symmetry == LayoutSymmetry::Radial) {
			const float angle = 2.0f * PI * static_cast<float>(index)
				/ static_cast<float>(std::max<std::size_t>(count, 1u));
			const float cosine = std::cos(angle);
			const float sine = std::sin(angle);
			const float xRadius = settings.hull.width * 0.52f + radius + socketHeight;
			const float yRadius = settings.hull.height * 0.52f + radius + socketHeight;
			const float stationZ = settings.hull.length * (0.22f - spread * 0.44f)
				+ static_cast<float>(variant) * radius * 0.08f;
			result.position = {cosine * xRadius, sine * yRadius, stationZ};
			result.supportRoot = {
				cosine * settings.hull.width * 0.48f,
				sine * settings.hull.height * 0.48f,
				stationZ
			};
			if (settings.design.weaponLayout.coverage == WeaponCoverage::Forward)
				result.forward = {0.0f, 0.0f, 1.0f};
			else
				result.forward = gen_model::gen_types::normalize(Point3{
					cosine,
					sine,
					settings.design.weaponLayout.coverage == WeaponCoverage::Broadside ? 0.32f : 0.0f
				});
			result.supportWidth = std::max(source.supportWidth, radius * 3.0f);
			result.supportHeight = std::max(source.supportHeight, radius * 2.4f);
			if (!preliminary.candidateSurfaces.empty())
				result.socketHeight = std::max(source.socketHeight, radius * 0.72f);
			if (source.requestedFacing.has_value())
				result.forward = gen_model::gen_types::normalize(*source.requestedFacing);
			moveOutsideUnitParentSphere(result);
			return result;
		}
		const float wingSpan = std::max(settings.wings.halfSpan, settings.hull.width * 0.70f);
		const float edgeClearanceScale = preliminary.metrics.selectedLayout == PropulsionLayout::TwinBoom
			? 0.65f
			: 1.8f;
		const float lateralLimit = wingSpan - radius * edgeClearanceScale;
		const float innerLateral = std::min(
			lateralLimit,
			std::max(settings.hull.width * 0.70f, settings.wings.rootX + radius * 3.4f)
		);
		const float outerLateral = std::max(innerLateral, lateralLimit);
		// Sparse batteries follow the swept leading-edge load path.  Additional
		// pairs move outboard instead of stacking fore/aft, which preserves a clear
		// muzzle sector and gives four-gun wings deliberate visual rhythm.
		const float lateralAmount = std::clamp(
			(pairCount <= 1u ? 0.62f : 0.35f + spread * 0.55f)
				+ architectureBias(preliminary.metrics.selectedLayout),
			0.20f,
			0.94f
		);
		const float lateralStep = std::max(
			radius * 0.12f,
			(outerLateral - innerLateral) * 0.18f
		);
		float lateral = std::clamp(
			innerLateral + (outerLateral - innerLateral) * lateralAmount
				+ static_cast<float>(variant) * lateralStep,
			innerLateral,
			outerLateral
		);
		if (!dense && paired && pairCount == 2u) {
			const float outerAmount = std::clamp(
				0.90f + architectureBias(preliminary.metrics.selectedLayout),
				0.20f,
				0.94f
			);
			const float batteryOuter = std::clamp(
				innerLateral + (outerLateral - innerLateral) * outerAmount
					+ static_cast<float>(variant) * radius * 0.12f,
				innerLateral,
				outerLateral
			);
			const float sweptPeerSeparation = source.barrelRadius * 2.0f
				+ std::sin(sweepRadians) * source.barrelLength
				+ radius * 0.12f;
			lateral = pairIndex == 0u
				? std::max(innerLateral, batteryOuter - sweptPeerSeparation)
				: batteryOuter;
		}
		const bool fighter = settings.layout.archetype == "patrol_fighter"
			|| settings.layout.archetype == "multirole"
			|| settings.layout.archetype == "heavy_fighter"
			|| settings.layout.archetype == "interceptor";
		const float generatedWingRootX = fighter
			? std::max(settings.wings.rootX * 0.75f, settings.hull.width * 0.28f)
			: std::max(settings.wings.rootX, settings.hull.width * 0.42f);
		const float wingAmount = std::clamp(
			(std::abs(lateral) - generatedWingRootX)
				/ std::max(settings.wings.halfSpan - generatedWingRootX, radius),
			0.0f,
			1.0f
		);
		const float easedWingAmount = smoothStep(wingAmount);
		const float localFront = settings.wings.rootFrontZ
			+ (settings.wings.tipFrontZ - settings.wings.rootFrontZ) * easedWingAmount;
		const float localRear = settings.wings.rootRearZ
			+ (settings.wings.tipRearZ - settings.wings.rootRearZ) * easedWingAmount;
		// Twin-boom fighters seat their battery in the swept wing's leading-edge
		// shoulder.  Moving it aft into the chord makes an inward/downward barrel
		// sweep cross the wing before it can leave the airframe.
		const float chordAmount = preliminary.metrics.selectedLayout == PropulsionLayout::TwinBoom
			? 0.0f
			: 0.12f + spread * 0.04f;
		float z = localFront + (localRear - localFront) * chordAmount
			+ static_cast<float>(variant) * radius * 0.08f;
		if (preliminary.metrics.selectedLayout == PropulsionLayout::TwinBoom)
			z += source.barrelRadius;
		const float wingRootTop = fighter
			? std::max(
				settings.wings.topY + 0.04f,
				settings.hull.height * 0.13f + settings.hull.crown * 0.40f
			)
			: settings.hull.height * 0.13f + settings.hull.crown * 0.72f;
		const float wingTipTop = settings.wings.topY + 0.05f;
		const float rootCamber = settings.layout.archetype == "interceptor" ? 0.20f : 0.16f;
		const float tipCamber = settings.layout.archetype == "interceptor" ? 0.08f : 0.06f;
		float integratedDeckY = wingRootTop
			+ (wingTipTop - wingRootTop) * easedWingAmount
			+ rootCamber + (tipCamber - rootCamber) * easedWingAmount;
		if (preliminary.metrics.selectedLayout == PropulsionLayout::TwinBoom
			|| preliminary.metrics.selectedLayout == PropulsionLayout::WingNacelles) {
			float nearestPodDistance = std::numeric_limits<float>::max();
			for (const auto& pod : preliminary.enginePods) {
				const float podDistance = std::abs(std::abs(pod.runtime.center.x) - lateral);
				const float longitudinalDistance = std::abs(pod.runtime.center.z - z);
				if (podDistance > pod.runtime.radius * 1.35f
					|| longitudinalDistance > pod.runtime.length * 0.62f)
					continue;
				if (podDistance >= nearestPodDistance)
					continue;
				nearestPodDistance = podDistance;
				integratedDeckY = std::max(
					integratedDeckY,
					pod.runtime.center.y + pod.runtime.radius * 1.15f
				);
			}
		}
		if (!dense) {
			const bool centerline = paired && count % 2u == 1u && index == count - 1u;
			result.position = centerline
				? Point3{
					0.0f,
					settings.hull.height * 0.50f + radius + socketHeight,
					settings.hull.length * 0.20f
				}
				: Point3{
					side * lateral,
					integratedDeckY + radius + socketHeight,
					z
				};
			result.forward = coverageDirection(
				settings.design.weaponLayout.coverage,
				side,
				1.0f,
				pairIndex
			);
			result.supportRoot = centerline
				? Point3{
					0.0f,
					settings.hull.height * 0.28f,
					settings.hull.length * 0.16f
				}
				: Point3{
					side * std::max(
						settings.wings.rootX,
						lateral - std::max(radius * 2.2f, settings.wings.shoulderWidth * 0.80f)
					),
					integratedDeckY - radius * 0.16f,
					z - radius * 0.10f
				};
		} else {
		// A fixed-forward dense battery cannot use an aft station: its complete
		// traverse cone must leave the ship in +Z.  Give the capital a second
		// lateral rail so all four bilateral pairs can live on the forward deck
		// without stacking at one x/y site.  Null-facing batteries retain the
		// distributed fore/aft coverage policy (Terminator's deliberate exception).
		const bool fixedForward = source.requestedFacing.has_value()
			&& source.requestedFacing->z > 0.5f;
		const std::size_t lateralRails = (pairCount >= 8u || (fixedForward && pairCount >= 4u)) ? 2u : 1u;
			const std::size_t lanes = lateralRails * 2u;
			const std::size_t stationCount = std::max<std::size_t>(
				(pairCount + lanes - 1u) / lanes,
				1u
			);
			const std::size_t lane = pairIndex % lanes;
			const std::size_t station = pairIndex / lanes;
			const std::size_t lateralRail = lane % lateralRails;
			const bool dorsal = (lane / lateralRails) == 0u;
			const float verticalSide = dorsal ? 1.0f : -1.0f;
			const float capitalLateral = std::min(
				std::max(settings.hull.width * 1.15f, radius * 6.0f + settings.wings.rootX),
				std::max(settings.wings.halfSpan - radius * 1.8f, settings.hull.width * 0.70f)
			);
			const float safeLateral = settings.wings.halfSpan
				+ std::sin(sweepRadians) * source.barrelLength
				+ radius * 1.35f;
			const float minimumSeparation = radius * 2.0f
				* std::max(settings.design.weaponLayout.minimumSeparationScale, 1.0f);
			const float railSpacing = std::max(minimumSeparation * 1.15f, radius * 3.2f);
			const float resolvedLateral = std::max(capitalLateral, safeLateral)
				+ static_cast<float>(lateralRail) * railSpacing
				+ static_cast<float>(variant) * radius * 0.12f;
		const float stationAmount = stationCount <= 1u
			? (fixedForward ? 0.0f : 0.5f)
			: static_cast<float>(station) / static_cast<float>(stationCount - 1u);
		const float railFront = settings.hull.length * 0.28f;
		const float railRear = -settings.hull.length * 0.30f;
		const float railZ = railFront + (railRear - railFront) * stationAmount
			+ static_cast<float>(variant) * radius * 0.08f;
			float machineryHalfHeight = settings.hull.height * 0.52f;
			for (const auto& module : preliminary.coreModules) {
				if (!module.protectedByEnvelope)
					continue;
				machineryHalfHeight = std::max(
					machineryHalfHeight,
					std::abs(module.center.y) + module.halfExtents.y
				);
			}
			const float batteryDeckHeight = machineryHalfHeight
				+ settings.design.moduleClearance + radius * 0.60f;
			result.position = {
				side * resolvedLateral,
				verticalSide * (batteryDeckHeight + radius + socketHeight),
				railZ
			};
			if (settings.design.weaponLayout.coverage == WeaponCoverage::Omnidirectional) {
				if (stationCount == 1u)
					result.forward = {side, 0.0f, 0.0f};
				else if (station == 0u)
					result.forward = {0.0f, 0.0f, 1.0f};
				else if (station + 1u == stationCount)
					result.forward = {0.0f, 0.0f, -1.0f};
				// Only the outside rail fires broadside.  An inboard broadside barrel
				// would pass through the next turret on the same deck; its vertical
				// sector completes the coverage without self-intersection.
				else if (lateralRail + 1u == lateralRails)
					result.forward = {side, 0.0f, 0.0f};
				else
					result.forward = {0.0f, verticalSide, 0.0f};
			} else {
				result.forward = coverageDirection(
					settings.design.weaponLayout.coverage,
					side,
					verticalSide,
					pairIndex
				);
			}
			result.supportRoot = {
				side * resolvedLateral,
				verticalSide * batteryDeckHeight,
				railZ
			};
		}
		result.supportWidth = std::max(source.supportWidth, radius * (dense ? 3.0f : 2.4f));
		result.supportHeight = std::max(source.supportHeight, radius * (dense ? 2.4f : 2.0f));
		if (source.requestedFacing.has_value())
			result.forward = gen_model::gen_types::normalize(*source.requestedFacing);
		moveOutsideUnitParentSphere(result);
		return result;
	}

	Module magazineFor(const MountSettings& mount, std::size_t index, bool dense) {
		const float volume = 4.0f * PI * mount.turretRadius * mount.turretRadius
			* mount.turretRadius / 3.0f * 1.7f
			+ PI * mount.barrelRadius * mount.barrelRadius * mount.barrelLength * 0.35f;
		Point3 center = mount.supportRoot * (dense ? 0.62f : 0.84f)
			+ mount.position * (dense ? 0.38f : 0.16f);
		const float verticalSide = mount.position.y < 0.0f ? -1.0f : 1.0f;
		center.y += verticalSide * mount.turretRadius * (dense ? 0.35f : -0.60f);
		const Point3 extents{
			std::max(mount.turretRadius * (dense ? 2.0f : 1.6f), mount.supportWidth * 0.62f),
			std::max(mount.turretRadius * 1.25f, mount.supportHeight * 0.60f),
			std::max(mount.turretRadius * 1.55f, mount.barrelLength * 0.18f)
		};
		return makeModule(
			ModuleKind::Magazine,
			dense ? ModuleShape::RadialWedge : ModuleShape::ChamferedBox,
			center,
			extents,
			volume,
			static_cast<int>(index)
		);
	}

	Module foundationFor(const MountSettings& mount, std::size_t index) {
		const Point3 center = (mount.supportRoot + mount.position) * 0.5f;
		return makeModule(
			ModuleKind::MountFoundation,
			ModuleShape::TaperedBeam,
			center,
			{
				std::max(mount.supportWidth * 0.55f, mount.turretRadius),
				std::max(mount.supportHeight * 0.55f, mount.turretRadius * 0.82f),
				std::max(distance(mount.supportRoot, mount.position) * 0.50f, mount.turretRadius)
			},
			mount.supportWidth * mount.supportHeight * mount.socketHeight,
			static_cast<int>(index)
		);
	}

	bool hardFeasible(
		const Settings& settings,
		const Layout& layout,
		std::string* diagnostic = nullptr
	) {
		auto reject = [&](const std::string& message) {
			if (diagnostic)
				*diagnostic = message;
			return false;
		};
		const float minimumScale = std::max(settings.design.weaponLayout.minimumSeparationScale, 1.0f);
		for (std::size_t left = 0; left < layout.mounts.size(); ++left) {
			const auto& mount = layout.mounts[left];
			const float parentClearance =
				gen_model::spaceship::weapon_layout::unitParentCollisionClearance(mount);
			if (parentClearance < 0.02f) {
				std::ostringstream message;
				message << "mount enters unit parent collision sphere index=" << left
					<< " position=" << pointText(mount.position)
					<< " turretRadius=" << mount.turretRadius
					<< " clearance=" << parentClearance;
				return reject(message.str());
			}
			const float sweepRadians = mount.traverseHalfAngleDegrees
				* 3.14159265358979323846f / 180.0f;
			const float outerX = std::max(settings.wings.halfSpan, settings.hull.width * 0.5f)
				+ std::sin(sweepRadians) * mount.barrelLength
				+ mount.turretRadius * 2.0f;
			if (!denseBattery(settings) && std::abs(mount.position.x) > outerX + EPSILON) {
				std::ostringstream message;
				message << "mount outside sparse structural reach index=" << left
					<< " position=" << pointText(mount.position) << " limit=" << outerX;
				return reject(message.str());
			}
			const Point3 barrelStart = mount.position + mount.forward
				* (mount.turretRadius + mount.barrelRadius * 1.2f);
			const Point3 barrelEnd = mount.position + mount.forward * mount.barrelLength;
			for (std::size_t right = left + 1u; right < layout.mounts.size(); ++right) {
				const auto& other = layout.mounts[right];
				const float required = (mount.turretRadius + other.turretRadius) * minimumScale;
				const float centerDistance = distance(mount.position, other.position);
				if (centerDistance < required) {
					std::ostringstream message;
					message << "turret envelopes overlap left=" << left << " right=" << right
						<< " leftPosition=" << pointText(mount.position)
						<< " rightPosition=" << pointText(other.position)
						<< " distance=" << centerDistance << " required=" << required;
					return reject(message.str());
				}
				if (pointSegmentDistance(other.position, barrelStart, barrelEnd)
					< mount.barrelRadius + other.turretRadius * 0.92f) {
					std::ostringstream message;
					message << "barrel centerline intersects turret left=" << left
						<< " right=" << right << " leftPosition=" << pointText(mount.position)
						<< " leftForward=" << pointText(mount.forward)
						<< " rightPosition=" << pointText(other.position);
					return reject(message.str());
				}
			}
		}
		return true;
	}

	float scoreLayout(const Settings& settings, Layout& layout) {
		if (layout.mounts.empty())
			return -std::numeric_limits<float>::max();
		float minimumDistance = std::numeric_limits<float>::max();
		float supportDistance = 0.0f;
		float xBalance = 0.0f;
		for (std::size_t index = 0; index < layout.mounts.size(); ++index) {
			const auto& mount = layout.mounts[index];
			supportDistance += distance(mount.supportRoot, mount.position);
			xBalance += mount.position.x;
			for (std::size_t other = index + 1u; other < layout.mounts.size(); ++other)
				minimumDistance = std::min(minimumDistance, distance(mount.position, layout.mounts[other].position));
		}
		layout.coverageScore = layout.mounts.size() * 1.4f;
		layout.separationScore = minimumDistance / std::max(settings.design.weaponLayout.minimumSeparationScale, 1.0f);
		layout.structuralScore = -supportDistance / static_cast<float>(layout.mounts.size());
		layout.integrationScore = -static_cast<float>(layout.modules.size()) * 0.01f;
		layout.balanceScore = -std::abs(xBalance);
		layout.totalScore = layout.coverageScore * 2.0f
			+ layout.separationScore * 1.5f
			+ layout.structuralScore * 1.2f
			+ layout.integrationScore
			+ layout.balanceScore * 2.0f;
		return layout.totalScore;
	}

	Layout makeCandidate(
		const Settings& settings,
		const gen_model::spaceship::design::PreliminaryDesign& preliminary,
		int variant
	) {
		Layout result;
		result.mounts.reserve(settings.mounts.size());
		result.modules.reserve(settings.mounts.size() * 2u);
		const bool dense = denseBattery(settings);
		for (std::size_t index = 0; index < settings.mounts.size(); ++index) {
			MountSettings mount = resolvedMount(settings, index, variant, preliminary);
			result.mounts.push_back(mount);
			result.modules.push_back(magazineFor(mount, index, dense));
			result.modules.push_back(foundationFor(mount, index));
		}
		if (!hardFeasible(settings, result))
			return result;
		scoreLayout(settings, result);
		return result;
	}
}

const gen_model::spaceship::weapon_layout::CandidateLayout&
gen_model::spaceship::weapon_layout::LayoutCandidates::best() const {
	if (ranked.empty())
		throw std::logic_error("Spaceship weapon layout has no feasible candidates");
	return ranked.front();
}

gen_model::spaceship::weapon_layout::LayoutCandidates
gen_model::spaceship::weapon_layout::plan(
	const Settings& settings,
	const design::PreliminaryDesign& preliminary
) {
	LayoutCandidates result;
	if (settings.design.weaponLayout.placement == PlacementMode::Manual) {
		Layout candidate;
		candidate.mounts = settings.mounts;
		candidate.modules.reserve(settings.mounts.size() * 2u);
		const bool dense = denseBattery(settings);
		for (std::size_t index = 0; index < settings.mounts.size(); ++index) {
			candidate.modules.push_back(magazineFor(settings.mounts[index], index, dense));
			candidate.modules.push_back(foundationFor(settings.mounts[index], index));
		}
		std::string diagnostic;
		if (!hardFeasible(settings, candidate, &diagnostic))
			throw std::invalid_argument("Manual spaceship weapon layout rejected: " + diagnostic);
		scoreLayout(settings, candidate);
		result.ranked.push_back(std::move(candidate));
		return result;
	}
	for (const int variant : {0, 1, -1, 2, -2}) {
		Layout candidate = makeCandidate(settings, preliminary, variant);
		std::string diagnostic;
		if (!hardFeasible(settings, candidate, &diagnostic)) {
			std::ostringstream message;
			message << "weapon layout variant " << variant << " rejected: " << diagnostic;
			result.rejectionDiagnostics.push_back(message.str());
			continue;
		}
		result.ranked.push_back(std::move(candidate));
	}
	std::stable_sort(result.ranked.begin(), result.ranked.end(), [](const Layout& left, const Layout& right) {
		return left.totalScore > right.totalScore;
	});
	if (result.ranked.empty()) {
		std::ostringstream message;
		message << "No feasible automatic weapon layout for spaceship";
		for (const auto& diagnostic : result.rejectionDiagnostics)
			message << " [" << diagnostic << ']';
		throw std::invalid_argument(message.str());
	}
	return result;
}

float gen_model::spaceship::weapon_layout::unitParentCollisionClearance(
	const gen_model::spaceship::MountSettings& mount
) {
	return length(mount.position) - 1.0f - mount.turretRadius;
}
