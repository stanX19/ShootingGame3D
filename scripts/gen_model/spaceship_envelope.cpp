#include "spaceship_envelope.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
	constexpr float EPSILON = 0.00001f;
	using Point3 = gen_model::gen_types::Point3;
	using Surface = gen_model::spaceship::detail::Surface;
	using MeshBuilder = gen_model::spaceship::detail::MeshBuilder;
	using ModuleKind = gen_model::spaceship::design::ModuleKind;
	using PropulsionLayout = gen_model::spaceship::PropulsionLayout;
	using BatteryStyle = gen_model::spaceship::BatteryStyle;
	using PlacementMode = gen_model::spaceship::PlacementMode;

	bool fighterArchetype(const gen_model::spaceship::Settings& settings) {
		return settings.layout.archetype == "patrol_fighter"
			|| settings.layout.archetype == "multirole"
			|| settings.layout.archetype == "heavy_fighter"
			|| settings.layout.archetype == "interceptor";
	}

	bool carrierSponsonBattery(const gen_model::spaceship::Settings& settings) {
		return settings.layout.archetype == "carrier"
			&& settings.design.weaponLayout.batteryStyle == BatteryStyle::External
			&& settings.mounts.size() <= 16u;
	}

	float requireSurfaceAnchor(std::optional<float> surfaceY, const char* detailName) {
		if (!surfaceY.has_value())
			throw std::runtime_error(
				std::string("Cannot seat spaceship ") + detailName
				+ " because no structural skin exists at its planned station"
			);
		return *surfaceY;
	}

	struct BatteryRail {
		float x = 0.0f;
		float y = 0.0f;
		float minimumZ = 0.0f;
		float maximumZ = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		float endMargin = 0.0f;
		float socketContactY = 0.0f;
	};

	void addBatteryRootFairing(
		MeshBuilder& builder,
		Point3 root,
		Point3 socket,
		float rootChord,
		float socketChord,
		float rootHeight,
		float socketHeight
	) {
		const std::array<float, 4> amounts{0.0f, 0.30f, 0.68f, 1.0f};
		std::vector<std::vector<Point3>> rings;
		rings.reserve(amounts.size());
		for (const float amount : amounts) {
			const Point3 center = root + (socket - root) * amount;
			const float halfChord = (rootChord + (socketChord - rootChord) * amount) * 0.5f;
			const float halfHeight = (rootHeight + (socketHeight - rootHeight) * amount) * 0.5f;
			// The broad Y/Z section is a deck wing, not a circular rod.  Its chord
			// spreads recoil along the rail station while the shallow height keeps the
			// complete forward firing cone above the structure.
			rings.push_back({
				{center.x, center.y, center.z - halfChord},
				{center.x, center.y + halfHeight * 0.68f, center.z - halfChord * 0.82f},
				{center.x, center.y + halfHeight, center.z},
				{center.x, center.y + halfHeight * 0.68f, center.z + halfChord * 0.82f},
				{center.x, center.y, center.z + halfChord},
				{center.x, center.y - halfHeight * 0.62f, center.z + halfChord * 0.82f},
				{center.x, center.y - halfHeight, center.z},
				{center.x, center.y - halfHeight * 0.62f, center.z - halfChord * 0.82f}
			});
		}
		builder.addClosedLoft(
			rings,
			Surface::Structure,
			Surface::Structure,
			Surface::Structure
		);
	}

	void addCarrierSponsonFairing(
		MeshBuilder& builder,
		Point3 root,
		Point3 tip,
		float rootWidth,
		float tipWidth,
		float rootTop,
		float rootBottom,
		float tipTop,
		float tipBottom
	) {
		const Point3 delta{tip.x - root.x, 0.0f, tip.z - root.z};
		const float deltaLength = std::sqrt(
			gen_model::gen_types::dot(delta, delta)
		);
		if (deltaLength <= EPSILON)
			return;
		const Point3 lateral{
			-delta.z / deltaLength,
			0.0f,
			delta.x / deltaLength
		};
		const float sideSign = std::abs(root.x + tip.x) > EPSILON
			? (root.x + tip.x >= 0.0f ? 1.0f : -1.0f)
			: 1.0f;
		const Point3 mirroredLateral{
			lateral.x * sideSign,
			0.0f,
			lateral.z * sideSign
		};
		const std::array<float, 4> amounts{0.0f, 0.30f, 0.68f, 1.0f};
		std::vector<std::vector<Point3>> rings;
		rings.reserve(amounts.size());
		for (const float amount : amounts) {
			const Point3 center = root + (tip - root) * amount;
			const Point3 centerXZ{center.x, 0.0f, center.z};
			const float width = rootWidth + (tipWidth - rootWidth) * amount;
			const float top = rootTop + (tipTop - rootTop) * amount;
			const float bottom = rootBottom + (tipBottom - rootBottom) * amount;
			const float halfWidth = width * 0.5f;
			const float crown = std::max(
				(top - bottom) * 0.20f,
				0.018f
			) * (1.0f - amount * 0.35f);
			rings.push_back({
				centerXZ - mirroredLateral * halfWidth
					+ Point3{0.0f, top, 0.0f},
				centerXZ - mirroredLateral * (halfWidth * 0.52f)
					+ Point3{0.0f, top + crown * 0.62f, 0.0f},
				centerXZ + Point3{0.0f, top + crown, 0.0f},
				centerXZ + mirroredLateral * (halfWidth * 0.52f)
					+ Point3{0.0f, top + crown * 0.62f, 0.0f},
				centerXZ + mirroredLateral * halfWidth
					+ Point3{0.0f, top, 0.0f},
				centerXZ + mirroredLateral * halfWidth
					+ Point3{0.0f, bottom, 0.0f},
				centerXZ + mirroredLateral * (halfWidth * 0.52f)
					+ Point3{0.0f, bottom - crown * 0.22f, 0.0f},
				centerXZ + Point3{0.0f, bottom - crown * 0.30f, 0.0f},
				centerXZ - mirroredLateral * (halfWidth * 0.52f)
					+ Point3{0.0f, bottom - crown * 0.22f, 0.0f},
				centerXZ - mirroredLateral * halfWidth
					+ Point3{0.0f, bottom, 0.0f}
			});
		}
		builder.addClosedLoft(
			rings,
			Surface::Structure,
			Surface::Structure,
			Surface::Structure
		);
	}

	float moveFairingAwayFromExhaust(
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::design::ModuleVolume& module,
		float centerY,
		float verticalRadius,
		float horizontalRadius,
		float longitudinalRadius
	) {
		for (const auto& engine : settings.engines) {
			const float rearZ = engine.center.z - engine.length * 0.5f - engine.nozzleDepth;
			const float throatFrontZ = rearZ + std::max(
				engine.nozzleDepth * 0.45f,
				engine.radius * 0.08f
			);
			const bool overlapsX = std::abs(module.center.x - engine.center.x)
				<= horizontalRadius + engine.radius * 0.86f;
			const bool overlapsZ = module.center.z + longitudinalRadius >= rearZ
				&& module.center.z - longitudinalRadius <= throatFrontZ;
			if (!overlapsX || !overlapsZ)
				continue;
			const float requiredOffset = engine.radius * 0.86f
				+ verticalRadius + settings.design.moduleClearance * 0.35f;
			const float offset = centerY - engine.center.y;
			if (std::abs(offset) >= requiredOffset)
				continue;
			centerY = engine.center.y
				+ (offset >= 0.0f ? requiredOffset : -requiredOffset);
		}
		return centerY;
	}

	void addWeaponBatteryRails(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings
	) {
		if (settings.design.weaponLayout.placement != PlacementMode::Auto
			|| settings.design.weaponLayout.batteryStyle != BatteryStyle::External)
			return;
		std::vector<BatteryRail> rails;
		for (const auto& mount : settings.mounts) {
			auto rail = std::find_if(rails.begin(), rails.end(), [&](const BatteryRail& candidate) {
				return std::abs(candidate.x - mount.supportRoot.x) <= 0.001f
					&& std::abs(candidate.y - mount.supportRoot.y) <= 0.001f;
			});
			if (rail == rails.end()) {
				const float verticalSide = mount.position.y < 0.0f ? -1.0f : 1.0f;
				rails.push_back({
					mount.supportRoot.x,
					mount.supportRoot.y,
					mount.supportRoot.z,
					mount.supportRoot.z,
					std::max(mount.supportWidth * 0.78f, mount.turretRadius * 1.8f),
					std::max(mount.supportHeight * 0.76f, mount.turretRadius * 1.45f),
					mount.turretRadius * 1.15f,
					mount.position.y - verticalSide * mount.turretRadius * 0.96f
				});
				continue;
			}
			rail->minimumZ = std::min(rail->minimumZ, mount.supportRoot.z);
			rail->maximumZ = std::max(rail->maximumZ, mount.supportRoot.z);
			rail->width = std::max(
				rail->width,
				std::max(mount.supportWidth * 0.78f, mount.turretRadius * 1.8f)
			);
			rail->height = std::max(
				rail->height,
				std::max(mount.supportHeight * 0.76f, mount.turretRadius * 1.45f)
			);
			rail->endMargin = std::max(rail->endMargin, mount.turretRadius * 1.15f);
			const float verticalSide = mount.position.y < 0.0f ? -1.0f : 1.0f;
			const float socketContactY =
				mount.position.y - verticalSide * mount.turretRadius * 0.96f;
			if (mount.position.y >= 0.0f)
				rail->socketContactY = std::max(rail->socketContactY, socketContactY);
			else
				rail->socketContactY = std::min(rail->socketContactY, socketContactY);
		}

		// Adjacent dense-battery rows can resolve to nearly coincident rails when
		// the optimizer nudges one row around the collision sphere.  They are one
		// physical deck in that case; merge the rail records before emitting them so
		// two long coplanar beams cannot occupy the same surface.
		for (std::size_t left = 0; left < rails.size(); ++left) {
			std::size_t right = left + 1u;
			while (right < rails.size()) {
				BatteryRail& first = rails[left];
				const BatteryRail& second = rails[right];
				const float mergeDistance = std::max(
					0.08f,
					std::min(first.width, second.width) * 0.25f
				);
				if (std::abs(first.y - second.y) > 0.001f
					|| std::abs(first.x - second.x) > mergeDistance) {
					++right;
					continue;
				}
				first.x = (first.x + second.x) * 0.5f;
				first.minimumZ = std::min(first.minimumZ, second.minimumZ);
				first.maximumZ = std::max(first.maximumZ, second.maximumZ);
				first.width = std::max(first.width, second.width);
				first.height = std::max(first.height, second.height);
				first.endMargin = std::max(first.endMargin, second.endMargin);
				rails.erase(rails.begin() + right);
			}
		}

		const bool carrierSponson = carrierSponsonBattery(settings);
		for (const auto& rail : rails) {
			const float forwardClearance = carrierSponson
				? std::max(0.06f, rail.endMargin * 0.52f)
				: -rail.endMargin;
			const float rearExtension = carrierSponson
				? std::max(rail.endMargin, rail.width * 0.80f)
				: rail.endMargin;
			builder.addTaperedBeam(
				{rail.x, rail.y, rail.minimumZ - rearExtension},
				{rail.x, rail.y, rail.maximumZ - forwardClearance},
				rail.width,
				rail.width * 0.94f,
				rail.height,
				Surface::Structure
			);
		}

		// One continuous load web per side/deck is enough to carry all rails.  A
		// separate capped beam between every adjacent rail creates coincident end
		// caps at their junctions (especially on the terminator's dense rows), so
		// route the web directly from the hull to the outermost rail instead.
		for (std::size_t index = 0; index < rails.size(); ++index) {
			const auto& rail = rails[index];
			bool hasOuterRail = false;
			for (std::size_t candidate = 0; candidate < rails.size(); ++candidate) {
				if (candidate == index
					|| rails[candidate].x * rail.x <= 0.0f
					|| std::abs(rails[candidate].y - rail.y) > 0.001f)
					continue;
				if (std::abs(rails[candidate].x) > std::abs(rail.x) + 0.001f) {
					hasOuterRail = true;
					break;
				}
			}
			if (hasOuterRail)
				continue;
			const float side = rail.x < 0.0f ? -1.0f : 1.0f;
			const float verticalSide = rail.y < 0.0f ? -1.0f : 1.0f;
			const float stationZ = (rail.minimumZ + rail.maximumZ) * 0.5f;
			const Point3 root{
				side * std::max(settings.wings.rootX, settings.hull.width * 0.46f),
				verticalSide * settings.hull.height * 0.34f,
				stationZ
			};
			const Point3 socket{rail.x, rail.y, stationZ};
			if (carrierSponson) {
				// The carrier deck is a broad swept sponson.  Its forward edge stops
				// just behind the socket, so the full forward firing cone leaves the
				// ship immediately instead of passing through a support rod or rail.
				const float frontClearance = std::max(
					0.06f, rail.endMargin * 0.52f
				);
				const float rootWidth = std::max(
					settings.hull.length * 0.46f,
					rail.endMargin * 4.0f
				);
				const float tipWidth = std::max(
					settings.hull.length * 0.11f,
					rail.endMargin * 1.8f
				);
				const float rootZ = stationZ - frontClearance - rootWidth * 0.5f;
				const float tipZ = stationZ - frontClearance - tipWidth * 0.5f;
				const float rootDeckTop = settings.hull.height * 0.52f
					+ settings.hull.crown;
				// Keep dorsal and ventral sponsons on their own hull shoulders.  If
				// both root sections cross the centre plane, their vertical side faces
				// become genuinely coplanar overlapping render surfaces.
				const float rootShoulder = settings.hull.height * 0.08f;
				const bool dorsal = rail.y >= 0.0f;
				const float rootTop = dorsal ? rootDeckTop : -rootShoulder;
				const float rootBottom = dorsal
					? rootShoulder
					: -rootDeckTop;
				const float tipTop = rail.socketContactY;
				const float tipBottom = tipTop - std::max(
					rail.height * 1.15f,
					settings.armorDepth * 1.8f
				);
				addCarrierSponsonFairing(
					builder,
					{root.x, 0.0f, rootZ},
					{socket.x, 0.0f, tipZ},
					rootWidth,
					tipWidth,
					rootTop,
					rootBottom,
					tipTop,
					tipBottom
				);
			} else {
				const float rootChord = std::max(
					settings.hull.length * 0.22f,
					rail.endMargin * 3.0f
				);
				addBatteryRootFairing(
					builder,
					root,
					socket,
					rootChord,
					rootChord * 0.70f,
					rail.height * 1.45f,
					rail.height * 1.05f
				);
			}
		}
	}

	bool addFuelTankFairing(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::design::ModuleVolume& module,
		std::optional<float> surfaceY
	) {
		if (module.kind != ModuleKind::FuelTank)
			return false;
		const bool fighter = fighterArchetype(settings);
		const std::array<float, 5> amounts{0.0f, 0.22f, 0.50f, 0.78f, 1.0f};
		const std::array<float, 5> scales = fighter
			? std::array<float, 5>{0.26f, 0.78f, 1.00f, 0.72f, 0.22f}
			: std::array<float, 5>{0.58f, 0.94f, 1.06f, 0.90f, 0.56f};
		const float fighterVerticalRadius = std::min(
			module.halfExtents.y * 0.32f,
			std::max(settings.hull.height * 0.08f, settings.armorDepth * 0.80f)
		);
		float centerY = fighter
			? requireSurfaceAnchor(surfaceY, "fuel housing") - fighterVerticalRadius * 0.45f
			: module.center.y;
		centerY = moveFairingAwayFromExhaust(
			settings,
			module,
			centerY,
			fighter ? fighterVerticalRadius * 1.06f : module.halfExtents.y * 1.06f,
			module.halfExtents.x * 1.06f,
			module.halfExtents.z * 1.06f
		);
		std::vector<std::vector<Point3>> rings;
		rings.reserve(amounts.size());
		for (std::size_t index = 0; index < amounts.size(); ++index) {
			const float z = module.center.z
				+ (amounts[index] * 2.0f - 1.0f) * module.halfExtents.z;
			const float xRadius = module.halfExtents.x * scales[index]
				* (fighter ? 0.82f : 1.0f);
			const float yRadius = (fighter ? fighterVerticalRadius : module.halfExtents.y)
				* scales[index];
			std::vector<Point3> ring;
			ring.reserve(10u);
			for (int segment = 0; segment < 10; ++segment) {
				const float angle = 2.0f * 3.14159265358979323846f
					* static_cast<float>(segment) / 10.0f;
				ring.push_back({
					module.center.x + std::cos(angle) * xRadius,
					centerY + std::sin(angle) * yRadius,
					z
				});
			}
			rings.push_back(std::move(ring));
		}
		const Surface surface = fighter ? Surface::Armor : Surface::Structure;
		builder.addClosedLoft(rings, surface, surface, surface);
		if (fighter) {
			const float stripHalfLength = module.halfExtents.z * 0.44f;
			const float stripWidth = std::max(
				settings.armorDepth * 0.72f,
				module.halfExtents.x * 0.22f
			);
			const float stripHeight = std::max(0.022f, settings.armorDepth * 0.22f);
			const float stripY = centerY + fighterVerticalRadius * 0.72f;
			builder.addTaperedBeam(
				{module.center.x, stripY, module.center.z - stripHalfLength},
				{module.center.x, stripY, module.center.z + stripHalfLength},
				stripWidth,
				stripWidth * 0.72f,
				stripHeight,
				Surface::Structure
			);
		}
		return true;
	}

	bool addReactorShield(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::design::ModuleVolume& module,
		std::optional<float> surfaceY
	) {
		if (module.kind != ModuleKind::Reactor)
			return false;
		const std::array<float, 5> amounts{0.0f, 0.22f, 0.50f, 0.78f, 1.0f};
		const std::array<float, 5> scales{0.48f, 0.92f, 1.08f, 0.90f, 0.46f};
		const bool fighter = fighterArchetype(settings);
		const float verticalRadius = std::max(
			settings.armorDepth * 1.25f,
			module.halfExtents.y * (fighter ? 0.16f : 0.24f)
		);
		float centerY = requireSurfaceAnchor(surfaceY, "reactor shield")
			- verticalRadius * (fighter ? 0.52f : 0.28f);
		centerY = moveFairingAwayFromExhaust(
			settings,
			module,
			centerY,
			verticalRadius * 1.08f,
			module.halfExtents.x * 1.08f,
			module.halfExtents.z * 0.96f
		);
		std::vector<std::vector<Point3>> rings;
		rings.reserve(amounts.size());
		for (std::size_t index = 0; index < amounts.size(); ++index) {
			const float z = module.center.z
				+ (amounts[index] * 2.0f - 1.0f) * module.halfExtents.z * 0.92f;
			std::vector<Point3> ring;
			ring.reserve(10u);
			for (int segment = 0; segment < 10; ++segment) {
				const float angle = 2.0f * 3.14159265358979323846f
					* static_cast<float>(segment) / 10.0f;
				ring.push_back({
					module.center.x + std::cos(angle) * module.halfExtents.x * scales[index],
					centerY + std::sin(angle) * verticalRadius * scales[index],
					z
				});
			}
			rings.push_back(std::move(ring));
		}
		builder.addClosedLoft(rings, Surface::Armor, Surface::Armor, Surface::Armor);
		const float stripHalfLength = module.halfExtents.z * 0.42f;
		const float stripWidth = std::max(
			settings.armorDepth * 0.86f,
			module.halfExtents.x * 0.24f
		);
		builder.addTaperedBeam(
			{module.center.x, centerY + verticalRadius * 0.76f,
				module.center.z - stripHalfLength},
			{module.center.x, centerY + verticalRadius * 0.76f,
				module.center.z + stripHalfLength},
			stripWidth,
			stripWidth * 0.78f,
			std::max(0.024f, settings.armorDepth * 0.24f),
			Surface::Structure
		);
		return true;
	}

	bool addRadiatorPanel(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::design::ModuleVolume& module,
		std::optional<float> surfaceY
	) {
		if (module.kind != ModuleKind::Radiator)
			return false;
		const bool fighter = fighterArchetype(settings);
		const float skinY = requireSurfaceAnchor(surfaceY, "radiator panel");
		// A separate fighter "frame" and panel occupied the same skin plane with
		// only a few millimetres of offset.  That pair z-fought in the renderer and
		// was not an actual second mechanical volume.  Keep one load-bearing panel;
		// its frame/vent relief belongs in the material normal atlas.
		const float panelHeight = fighter
			? std::max(module.halfExtents.y * 1.40f, 0.035f)
			: module.halfExtents.y * 2.0f;
		float panelY = skinY + panelHeight * 0.20f;
		panelY = moveFairingAwayFromExhaust(
			settings,
			module,
			panelY,
			panelHeight * 0.60f,
			module.halfExtents.x * 2.45f,
			module.halfExtents.z * 1.08f
		);
		builder.addTaperedBeam(
			{module.center.x, panelY, module.center.z - module.halfExtents.z},
			{module.center.x, panelY, module.center.z + module.halfExtents.z},
			module.halfExtents.x * 2.0f,
			module.halfExtents.x * 1.74f,
			panelHeight,
			Surface::Structure
		);
		return true;
	}

	bool addServiceAccessPanel(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::design::ModuleVolume& module,
		std::optional<float> surfaceY
	) {
		if (module.kind != ModuleKind::ServiceBay)
			return false;
		const bool fighter = fighterArchetype(settings);
		const float height = std::max(settings.armorDepth * 0.42f, 0.035f);
		float y = requireSurfaceAnchor(surfaceY, "service access panel")
			// Fighter service access is a flush dorsal hatch.  Raising a full-width
			// beam above the pressure skin made the common service module look like a
			// second hull or a displaced rear block.  Capital ships retain the raised
			// deck lip because their crews need a walkable service spine.
			+ height * (fighter ? 0.04f : 0.20f);
		y = moveFairingAwayFromExhaust(
			settings,
			module,
			y,
			height * (fighter ? 0.38f : 0.60f),
			module.halfExtents.x * (fighter ? 1.12f : 1.70f),
			module.halfExtents.z * (fighter ? 0.72f : 0.86f)
		);
		builder.addTaperedBeam(
			{module.center.x, y, module.center.z - module.halfExtents.z * (fighter ? 0.72f : 0.86f)},
			{module.center.x, y, module.center.z + module.halfExtents.z * (fighter ? 0.72f : 0.86f)},
			module.halfExtents.x * (fighter ? 1.12f : 1.70f),
			module.halfExtents.x * (fighter ? 0.92f : 1.22f),
			height * (fighter ? 0.72f : 1.0f),
			Surface::Structure
		);
		return true;
	}

	bool addPodRootFairing(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		const gen_model::spaceship::design::EnginePod& pod,
		PropulsionLayout layout
	) {
		const Point3 center = pod.runtime.center;
		Point3 root{
			center.x * 0.28f,
			center.y * 0.45f,
			center.z + pod.runtime.length * 0.18f
		};
		Point3 end = center + Point3{0.0f, 0.0f, pod.runtime.length * 0.34f};
		if (layout == PropulsionLayout::TwinBoom) {
			if (fighterArchetype(settings)) {
				// A fighter twin-boom pod is carried from its wing shoulder, not from
				// the extreme tail through the cockpit.  The old endpoints created a
				// long, detached bar across the entire aft silhouette (and a near-
				// coplanar cap at the wing root).  Keep this fairing local to the pod's
				// forward shoulder so the wing itself owns the load path.
				root = {
					center.x * 0.45f,
					center.y + pod.runtime.radius * 0.15f,
					center.z + pod.runtime.length * 0.16f
				};
				end = {
					center.x,
					center.y + pod.runtime.radius * 0.16f,
					center.z + pod.runtime.length * 0.42f
				};
			} else {
				root = {center.x, center.y, -settings.hull.length * 0.47f};
				end = {center.x, center.y + pod.runtime.radius * 0.10f, settings.hull.length * 0.26f};
			}
		} else if (layout == PropulsionLayout::WingNacelles) {
			root = {
				std::copysign(settings.hull.width * 0.34f, center.x),
				center.y + pod.runtime.radius * 0.18f,
				center.z + pod.runtime.length * 0.44f
			};
		} else if (layout == PropulsionLayout::SpineCluster) {
			root = {center.x * 0.20f, -settings.hull.height * 0.08f, settings.hull.length * 0.12f};
		} else if (layout == PropulsionLayout::DistributedAft
			|| layout == PropulsionLayout::CapitalSideBlocks) {
			root = {
				std::copysign(settings.hull.width * 0.42f, center.x),
				center.y * 0.55f,
				center.z + pod.runtime.length * 0.24f
			};
			if (layout == PropulsionLayout::DistributedAft) {
				// Distributed pods share a diagonal aft bank.  A broad trunk aimed
				// at the rear centreline would sweep through the neighbouring nozzle
				// apertures, so route each load path from the hull to the pod's
				// forward shoulder instead.
				root = {
					std::copysign(settings.hull.width * 0.42f, center.x),
					center.y + pod.runtime.radius * 0.20f,
					center.z + pod.runtime.length * 0.42f
				};
				end = {
					center.x,
					center.y + pod.runtime.radius * 0.14f,
					center.z + pod.runtime.length * 0.42f
				};
			}
		}
		const float rootWidth = std::max(
			settings.wings.shoulderWidth * (fighterArchetype(settings) ? 1.65f : 2.4f),
			pod.runtime.radius * (fighterArchetype(settings) ? 2.55f : 3.8f)
		);
		const float endWidth = std::max(
			pod.runtime.radius * (fighterArchetype(settings) ? 1.85f : 2.6f),
			rootWidth * 0.62f
		);
		const float height = std::max(
			settings.armorDepth * (fighterArchetype(settings) ? 1.7f : 2.8f),
			pod.runtime.radius * (fighterArchetype(settings) ? 0.72f : 1.6f)
		);
		if (gen_model::gen_types::dot(center - root, center - root) <= EPSILON)
			return false;
		builder.addTaperedBeam(
			root,
			end,
			rootWidth,
			endWidth,
			height,
			fighterArchetype(settings) ? Surface::Armor : Surface::Structure
		);
		// The narrow raised channel used to sit within the fairing's depth buffer
		// envelope and flicker as a second coplanar shell.  The material atlas carries
		// this service seam now; keep one solid load fairing in the render mesh.
		return true;
	}

	void addArchitectureFrame(
		MeshBuilder& builder,
		const gen_model::spaceship::Settings& settings,
		PropulsionLayout layout
	) {
		if (layout == PropulsionLayout::SpineCluster) {
			builder.addTaperedBeam(
				{0.0f, -settings.hull.height * 0.08f, -settings.hull.length * 0.47f},
				{0.0f, settings.hull.height * 0.10f, settings.hull.length * 0.30f},
				settings.layout.primarySpineWidth * 1.75f,
				settings.layout.primarySpineWidth * 0.88f,
				settings.hull.height * 0.54f,
				Surface::Structure
			);
			return;
		}
		if (layout == PropulsionLayout::DistributedAft) {
			builder.addTaperedBeam(
				{-settings.hull.width * 0.64f, -settings.hull.height * 0.18f, -settings.hull.length * 0.34f},
				{settings.hull.width * 0.64f, -settings.hull.height * 0.18f, -settings.hull.length * 0.34f},
				settings.hull.length * 0.30f,
				settings.hull.length * 0.30f,
				settings.hull.height * 0.72f,
				Surface::Structure
			);
			return;
		}
		if (layout != PropulsionLayout::CapitalSideBlocks)
			return;
		for (const float side : {-1.0f, 1.0f})
			builder.addTaperedBeam(
				{side * settings.hull.width * 0.68f, -settings.hull.height * 0.10f, -settings.hull.length * 0.48f},
				{side * settings.hull.width * 0.64f, settings.hull.height * 0.06f, settings.hull.length * 0.24f},
				settings.hull.width * 0.46f,
				settings.hull.width * 0.34f,
				settings.hull.height * 0.74f,
				Surface::Structure
			);
	}

	void addMagazineFairing(
		MeshBuilder& builder,
		const gen_model::spaceship::design::ModuleVolume& module,
		const gen_model::spaceship::design::DesignPlan& plan
	) {
		if (module.kind != ModuleKind::Magazine || module.ownerIndex < 0
			|| static_cast<std::size_t>(module.ownerIndex) >= plan.resolvedMounts.size())
			return;
		const auto& mount = plan.resolvedMounts[static_cast<std::size_t>(module.ownerIndex)];
		const float verticalSide = mount.position.y < 0.0f ? -1.0f : 1.0f;
		const Point3 socketBase = mount.position - Point3{
			0.0f,
			verticalSide * (mount.turretRadius + mount.socketHeight * 0.38f),
			0.0f
		};
		if (gen_model::gen_types::dot(socketBase - module.center, socketBase - module.center) <= EPSILON)
			return;
		float nearestSocketSpacing = std::numeric_limits<float>::max();
		for (std::size_t index = 0; index < plan.resolvedMounts.size(); ++index) {
			if (static_cast<int>(index) == module.ownerIndex)
				continue;
			const Point3 delta = plan.resolvedMounts[index].position - mount.position;
			const float distance = std::sqrt(gen_model::gen_types::dot(delta, delta));
			if (distance > EPSILON)
				nearestSocketSpacing = std::min(nearestSocketSpacing, distance);
		}
		// External batteries share a rail/deck.  Their individual magazine
		// fairings are only the local load saddles; allowing each module's full
		// volume to reach the socket makes adjacent saddles overlap and produces
		// coplanar render faces.  Keep a deterministic clearance to the nearest
		// socket and let the shared rail carry the remaining magazine volume.
		const float saddleWidthLimit = nearestSocketSpacing < std::numeric_limits<float>::max()
			? nearestSocketSpacing * 0.72f
			: std::numeric_limits<float>::max();
		const float saddleHeightLimit = nearestSocketSpacing < std::numeric_limits<float>::max()
			? nearestSocketSpacing * 0.72f
			: std::numeric_limits<float>::max();
		const float saddleWidth = std::min(
			std::max(module.halfExtents.x * 2.0f, mount.supportWidth * 1.45f),
			saddleWidthLimit
		);
		const float saddleHeight = std::min(
			std::max(module.halfExtents.y * 2.0f, mount.supportHeight * 1.25f),
			saddleHeightLimit
		);
		builder.addTaperedBeam(
			module.center,
			socketBase,
			saddleWidth,
			std::min(
				std::max(mount.supportWidth * 1.15f, mount.turretRadius * 2.2f),
				saddleWidthLimit
			),
			saddleHeight,
			Surface::Structure,
			module.ownerIndex
		);
	}

}

gen_model::spaceship::SystemDetailReport
gen_model::spaceship::envelope::appendFunctionalFairings(
	detail::MeshBuilder& builder,
	const Settings& settings,
	const design::DesignPlan& plan
) {
	SystemDetailReport result;
	addArchitectureFrame(builder, settings, plan.metrics.selectedLayout);
	std::vector<std::optional<float>> surfaceAnchors;
	surfaceAnchors.reserve(plan.coreModules.size());
	for (const auto& module : plan.coreModules) {
		const float sampleX = std::round(std::abs(module.center.x) * 10000.0f) / 10000.0f;
		const float sampleZ = std::round(module.center.z * 10000.0f) / 10000.0f;
		surfaceAnchors.push_back(builder.topStructuralSurfaceY(sampleX, sampleZ));
	}
	for (const auto& pod : plan.enginePods)
		if (addPodRootFairing(builder, settings, pod, plan.metrics.selectedLayout))
			++result.feedTrunks;
	addWeaponBatteryRails(builder, settings);
	if (settings.design.weaponLayout.batteryStyle == BatteryStyle::External
		&& !carrierSponsonBattery(settings))
		for (const auto& module : plan.weaponModules)
			addMagazineFairing(builder, module, plan);
	for (std::size_t index = 0; index < plan.coreModules.size(); ++index) {
		const auto& module = plan.coreModules[index];
		const auto surfaceY = surfaceAnchors[index];
		if (addFuelTankFairing(builder, settings, module, surfaceY))
			++result.fuelHousings;
		else if (addReactorShield(builder, settings, module, surfaceY))
			++result.reactorShields;
		else if (addRadiatorPanel(builder, settings, module, surfaceY))
			++result.radiatorPanels;
		else if (addServiceAccessPanel(builder, settings, module, surfaceY))
			++result.serviceAccessPanels;
	}
	return result;
}
