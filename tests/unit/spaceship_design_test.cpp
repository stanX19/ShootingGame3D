#include "catch2/catch_amalgamated.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

#include "gen_model/spaceship_config.hpp"
#include "gen_model/spaceship_design.hpp"
#include "gen_model/spaceship_weapon_layout.hpp"

namespace {
	using Settings = gen_model::spaceship::Settings;

	Settings profile(const std::string& id) {
		for (const auto& settings : gen_model::spaceship::loadCatalog("assets/config/spaceships.json"))
			if (settings.id == id)
				return settings;
		throw std::logic_error("missing spaceship profile: " + id);
	}

	float expectedShapeVolume(
		gen_model::spaceship::design::ModuleShape shape,
		gen_model::gen_types::Point3 extents
	) {
		constexpr float pi = 3.14159265358979323846f;
		switch (shape) {
			case gen_model::spaceship::design::ModuleShape::FacetedCapsule:
			case gen_model::spaceship::design::ModuleShape::Ellipsoid:
			case gen_model::spaceship::design::ModuleShape::ShieldedSphere:
				return 4.0f * pi * extents.x * extents.y * extents.z / 3.0f;
			case gen_model::spaceship::design::ModuleShape::CappedCylinder:
			case gen_model::spaceship::design::ModuleShape::AxialFrustum:
				return pi * extents.x * extents.y * extents.z * 2.0f;
			default:
				return 8.0f * extents.x * extents.y * extents.z;
		}
	}
}

TEST_CASE("component shape selection is deterministic and role-correct", "[spaceship-design]")
{
	const auto first = gen_model::spaceship::design::planCore(profile("player"));
	const auto second = gen_model::spaceship::design::planCore(profile("player"));
	REQUIRE(first == second);
	for (const auto& module : first.coreModules) {
		REQUIRE(module.actualVolume >= module.requiredVolume);
		REQUIRE(module.actualVolume == Catch::Approx(expectedShapeVolume(module.shape, module.halfExtents)));
		REQUIRE(gen_model::spaceship::design::shapeAllowedFor(module.kind, module.shape));
	}
}

TEST_CASE("direct core planning tolerates an empty engine template", "[spaceship-design]")
{
	auto settings = profile("basic");
	settings.engines.clear();
	const auto plan = gen_model::spaceship::design::planCore(settings);
	REQUIRE(plan.enginePods.empty());
	REQUIRE(plan.metrics.availableThrust == Catch::Approx(0.0f));
	REQUIRE(plan.metrics.requiredThrust > 0.0f);
}

TEST_CASE("engineering plans include fuel power service and cooling systems", "[spaceship-design]")
{
	std::size_t basicRadiators = 0u;
	std::size_t mothershipRadiators = 0u;
	for (const std::string id : {"basic", "elite", "fastElite", "player", "terminator", "mothership"}) {
		const auto settings = profile(id);
		const auto plan = gen_model::spaceship::design::planCore(settings);
		const auto countKind = [&](gen_model::spaceship::design::ModuleKind kind) {
			return static_cast<std::size_t>(std::count_if(
				plan.coreModules.begin(), plan.coreModules.end(),
				[kind](const auto& module) { return module.kind == kind; }
			));
		};
		CAPTURE(id);
		REQUIRE(countKind(gen_model::spaceship::design::ModuleKind::FuelTank) == plan.enginePods.size());
		REQUIRE(countKind(gen_model::spaceship::design::ModuleKind::Reactor) == 1u);
		REQUIRE(countKind(gen_model::spaceship::design::ModuleKind::ServiceBay) == 1u);
		REQUIRE(countKind(gen_model::spaceship::design::ModuleKind::Radiator) >= 2u);
		REQUIRE(countKind(gen_model::spaceship::design::ModuleKind::Radiator) % 2u == 0u);
		for (const auto& fuel : plan.coreModules) {
			if (fuel.kind != gen_model::spaceship::design::ModuleKind::FuelTank)
				continue;
			const auto engine = std::find_if(
				plan.coreModules.begin(), plan.coreModules.end(),
				[&fuel](const auto& module) {
					return module.kind == gen_model::spaceship::design::ModuleKind::EngineCore
						&& module.ownerIndex == fuel.ownerIndex;
				}
			);
			REQUIRE(engine != plan.coreModules.end());
			REQUIRE(fuel.center.y == Catch::Approx(engine->center.y).margin(0.001f));
			REQUIRE(fuel.center.z > engine->center.z);
		}
		if (id != "terminator" && id != "mothership")
			for (const auto& module : plan.coreModules) {
				if (module.kind != gen_model::spaceship::design::ModuleKind::Radiator)
					continue;
				const float skinTop = settings.hull.height * 0.410f + settings.hull.crown;
				REQUIRE(module.center.y - module.halfExtents.y
					<= skinTop + settings.armorDepth * 0.40f);
				REQUIRE(module.center.y + module.halfExtents.y >= skinTop);
			}
		if (id == "basic")
			basicRadiators = countKind(gen_model::spaceship::design::ModuleKind::Radiator);
		if (id == "mothership")
			mothershipRadiators = countKind(gen_model::spaceship::design::ModuleKind::Radiator);
	}
	REQUIRE(mothershipRadiators > basicRadiators);
}

TEST_CASE("completion packages weapon modules into the structural graph", "[spaceship-design]")
{
	const auto settings = profile("basic");
	const auto preliminary = gen_model::spaceship::design::planCore(settings);
	const auto candidate = gen_model::spaceship::weapon_layout::plan(settings, preliminary).best();
	const auto plan = gen_model::spaceship::design::complete(
		settings, preliminary, candidate.modules, candidate.mounts
	);
	REQUIRE(plan.weaponModules.size() == candidate.modules.size());
	for (const auto& module : plan.weaponModules) {
		REQUIRE(module.actualVolume == Catch::Approx(expectedShapeVolume(module.shape, module.halfExtents)));
		REQUIRE(module.actualVolume >= module.requiredVolume);
	}
	REQUIRE(plan.coreModules.size() == preliminary.coreModules.size() + candidate.modules.size());
	REQUIRE(plan.nodes.size() >= plan.coreModules.size());
	for (std::size_t index = preliminary.coreModules.size(); index < plan.coreModules.size(); ++index) {
		const auto node = std::find_if(plan.nodes.begin(), plan.nodes.end(), [index](const auto& item) {
			return item.moduleIndex == static_cast<int>(index);
		});
		REQUIRE(node != plan.nodes.end());
	}
	const auto audit = gen_model::spaceship::design::audit(settings, plan);
	REQUIRE(audit.disconnectedNodes == 0u);
}

TEST_CASE("completion recouples mass and machinery metrics", "[spaceship-design]")
{
	const auto settings = profile("basic");
	const auto preliminary = gen_model::spaceship::design::planCore(settings);
	const auto candidate = gen_model::spaceship::weapon_layout::plan(settings, preliminary).best();
	const auto plan = gen_model::spaceship::design::complete(
		settings, preliminary, candidate.modules, candidate.mounts
	);
	float moduleMass = 0.0f;
	for (const auto& module : plan.coreModules)
		moduleMass += std::max(module.mass, 0.0f);
	REQUIRE(plan.metrics.massProxy == Catch::Approx(moduleMass));
	REQUIRE(plan.metrics.requiredThrust == Catch::Approx(
		moduleMass * std::max(settings.design.targetAcceleration, 0.05f)
	));
	REQUIRE(plan.metrics.engineVolume > 0.0f);
	REQUIRE(plan.metrics.fuelVolume > 0.0f);
	REQUIRE(plan.metrics.reactorVolume > 0.0f);
	float podCapacity = 0.0f;
	for (const auto& pod : plan.enginePods)
		podCapacity += pod.thrustCapacity;
	REQUIRE(plan.metrics.availableThrust == Catch::Approx(podCapacity));
}

TEST_CASE("audit checks graph reachability, centroids, thermal and access gates", "[spaceship-design]")
{
	const auto settings = profile("basic");
	const auto preliminary = gen_model::spaceship::design::planCore(settings);
	const auto candidate = gen_model::spaceship::weapon_layout::plan(settings, preliminary).best();
	const auto baseline = gen_model::spaceship::design::complete(
		settings, preliminary, candidate.modules, candidate.mounts
	);

	SECTION("unreachable node") {
		auto plan = baseline;
		plan.links.clear();
		REQUIRE(gen_model::spaceship::design::audit(settings, plan).disconnectedNodes > 0u);
	}

	SECTION("centroid tolerance") {
		auto plan = baseline;
		plan.metrics.massCenter.x = settings.dimensions.width * 0.03f;
		REQUIRE(gen_model::spaceship::design::audit(settings, plan).centroidViolations > 0u);
	}

	SECTION("thermal overlap") {
		auto plan = baseline;
		auto engine = std::find_if(plan.coreModules.begin(), plan.coreModules.end(), [](const auto& item) {
			return item.kind == gen_model::spaceship::design::ModuleKind::EngineCore;
		});
		auto cockpit = std::find_if(plan.coreModules.begin(), plan.coreModules.end(), [](const auto& item) {
			return item.kind == gen_model::spaceship::design::ModuleKind::Cockpit;
		});
		REQUIRE(engine != plan.coreModules.end());
		REQUIRE(cockpit != plan.coreModules.end());
		cockpit->center = engine->center;
		REQUIRE(gen_model::spaceship::design::audit(settings, plan).thermalViolations > 0u);
	}

	SECTION("access blocker") {
		auto plan = baseline;
		auto first = std::find_if(plan.coreModules.begin(), plan.coreModules.end(), [](const auto& item) {
			return item.kind == gen_model::spaceship::design::ModuleKind::Magazine;
		});
		REQUIRE(first != plan.coreModules.end());
		first->accessDirection = {0.0f, 0.0f, 1.0f};
		const auto originalCenter = first->center;
		const auto blocker = std::find_if(plan.coreModules.begin(), plan.coreModules.end(),
			[first](const auto& item) { return &item != &*first; });
		REQUIRE(blocker != plan.coreModules.end());
		blocker->center = originalCenter + gen_model::gen_types::Point3{
			0.0f,
			0.0f,
			first->halfExtents.z + settings.design.moduleClearance * 0.25f + 0.06f
		};
		blocker->halfExtents = {0.12f, 0.12f, 0.12f};
		blocker->ownerIndex = -999;
		REQUIRE(gen_model::spaceship::design::audit(settings, plan).accessViolations > 0u);
	}
}

TEST_CASE("audit reports pod capacity and fit failures without inflating capacity", "[spaceship-design]")
{
	const auto settings = profile("basic");
	const auto preliminary = gen_model::spaceship::design::planCore(settings);
	const auto candidate = gen_model::spaceship::weapon_layout::plan(settings, preliminary).best();
	auto plan = gen_model::spaceship::design::complete(
		settings, preliminary, candidate.modules, candidate.mounts
	);
	REQUIRE_FALSE(plan.enginePods.empty());
	plan.enginePods.front().thrustCapacity = 0.0f;
	const auto audit = gen_model::spaceship::design::audit(settings, plan);
	REQUIRE(audit.podCapacityViolations > 0u);
	REQUIRE(audit.propulsionViolations > 0u);
}

TEST_CASE("mass acceleration and endurance scale machinery", "[spaceship-design]")
{
	auto baseline = profile("basic");
	auto heavy = baseline;
	heavy.design.armorMassScale *= 1.5f;
	auto faster = baseline;
	faster.design.targetAcceleration *= 1.5f;
	auto longerRange = baseline;
	longerRange.design.endurance *= 1.5f;
	const auto basePlan = gen_model::spaceship::design::planCore(baseline);
	REQUIRE(gen_model::spaceship::design::planCore(heavy).metrics.engineVolume > basePlan.metrics.engineVolume);
	REQUIRE(gen_model::spaceship::design::planCore(heavy).metrics.fuelVolume > basePlan.metrics.fuelVolume);
	REQUIRE(gen_model::spaceship::design::planCore(faster).metrics.engineVolume > basePlan.metrics.engineVolume);
	REQUIRE(gen_model::spaceship::design::planCore(longerRange).metrics.fuelVolume > basePlan.metrics.fuelVolume);
}

TEST_CASE("automatic weapon layout is indexed, deterministic and balanced", "[spaceship-weapon-layout]")
{
	const auto settings = profile("player");
	const auto preliminary = gen_model::spaceship::design::planCore(settings);
	const auto first = gen_model::spaceship::weapon_layout::plan(settings, preliminary);
	const auto second = gen_model::spaceship::weapon_layout::plan(settings, preliminary);
	REQUIRE(first.best() == second.best());
	REQUIRE(first.best().mounts.size() == settings.mounts.size());
	float xBalance = 0.0f;
	for (const auto& mount : first.best().mounts)
		xBalance += mount.position.x;
	REQUIRE(xBalance == Catch::Approx(0.0f).margin(0.001f));
	for (std::size_t index = 0; index < first.best().mounts.size(); ++index)
		REQUIRE(first.best().mounts[index].id == settings.mounts[index].id);
}

TEST_CASE("automatic layout responds to architecture without ID rules", "[spaceship-weapon-layout]")
{
	auto player = profile("player");
	const auto playerPlan = gen_model::spaceship::design::planCore(player);
	const auto wing = gen_model::spaceship::weapon_layout::plan(player, playerPlan).best();
	player.design.propulsionLayout = gen_model::spaceship::PropulsionLayout::TwinBoom;
	const auto boom = gen_model::spaceship::weapon_layout::plan(
		player, gen_model::spaceship::design::planCore(player)
	).best();
	REQUIRE(wing.mounts != boom.mounts);
	for (auto& mount : player.mounts)
		mount.id = "diagnostic-only";
	const auto relabeled = gen_model::spaceship::weapon_layout::plan(
		player, gen_model::spaceship::design::planCore(player)
	).best();
	for (std::size_t index = 0; index < boom.mounts.size(); ++index)
		REQUIRE(relabeled.mounts[index].position == boom.mounts[index].position);
}

TEST_CASE("sparse fighter mounts stay seated on their structural surface", "[spaceship-weapon-layout]")
{
	for (const char* id : {"player", "basic", "elite", "fastElite"}) {
		const auto settings = profile(id);
		const auto preliminary = gen_model::spaceship::design::planCore(settings);
		const auto layout = gen_model::spaceship::weapon_layout::plan(settings, preliminary).best();
		for (const auto& mount : layout.mounts) {
			const float deckSide = mount.position.y < 0.0f ? -1.0f : 1.0f;
			const float bearingBaseY = mount.position.y - deckSide * mount.turretRadius;
			const float verticalGap = deckSide * (bearingBaseY - mount.supportRoot.y);
			CAPTURE(id, mount.id, verticalGap, mount.socketHeight);
			REQUIRE(verticalGap >= -0.001f);
			REQUIRE(verticalGap <= mount.socketHeight + mount.turretRadius * 0.20f + 0.001f);
			REQUIRE(std::abs(mount.position.x - mount.supportRoot.x)
				<= std::max(mount.supportWidth, mount.turretRadius * 2.4f) + 0.001f);
			REQUIRE(std::abs(mount.position.z - mount.supportRoot.z)
				<= mount.turretRadius * 0.20f + 0.001f);
		}
	}
}
