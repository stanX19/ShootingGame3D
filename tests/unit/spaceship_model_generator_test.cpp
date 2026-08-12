#include "catch2/catch_amalgamated.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "gen_model/spaceship_config.hpp"
#include "gen_model/spaceship_generator.hpp"
#include "gen_model/spaceship_mesh.hpp"
#include "gen_model/spaceship_topology.hpp"
#include "gen_model/spaceship_weapon_layout.hpp"
#include "json.hpp"

namespace {
	using Settings = gen_model::spaceship::Settings;
	using Point3 = gen_model::gen_types::Point3;

	float length(Point3 point) {
		return std::sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
	}

	Point3 faceNormal(const gen_model::gen_types::MeshData& mesh, const gen_model::gen_types::Triangle& triangle) {
		const Point3& a = mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])];
		const Point3& b = mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])];
		const Point3& c = mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])];
		return gen_model::gen_types::cross(b - a, c - a);
	}

	std::vector<Settings> catalog() {
		return gen_model::spaceship::loadCatalog("assets/config/spaceships.json");
	}

	bool isFighter(const Settings& settings) {
		return settings.layout.archetype == "patrol_fighter"
			|| settings.layout.archetype == "multirole"
			|| settings.layout.archetype == "heavy_fighter"
			|| settings.layout.archetype == "interceptor";
	}

	gen_model::gen_types::MeshData tetrahedron() {
		gen_model::gen_types::MeshData mesh;
		mesh.positions = {
			{0.0f, 0.0f, 0.0f},
			{1.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 1.0f}
		};
		mesh.texcoords.assign(4, {0.0f, 0.0f});
		mesh.normals.assign(4, {0.0f, 0.0f, 1.0f});
		const auto triangle = [](std::array<int, 3> indices) {
			return gen_model::gen_types::Triangle{indices, indices, indices};
		};
		mesh.triangles = {
			triangle({0, 2, 1}),
			triangle({0, 1, 3}),
			triangle({1, 2, 3}),
			triangle({0, 3, 2})
		};
		return mesh;
	}

	Settings reducedPlayerSettings() {
		const auto ships = catalog();
		for (const Settings& settings : ships) {
			if (settings.id == "player") {
				Settings reduced = settings;
				reduced.textureWidth = 64;
				reduced.textureHeight = 64;
				return reduced;
			}
		}
		throw std::logic_error("player profile is missing from spaceship catalog");
	}

	float normalSlopeP95(const gen_model::gen_types::TextureData& texture) {
		std::array<std::size_t, 256> histogram{};
		std::size_t samples = 0u;
		for (std::size_t offset = 0; offset + 3u < texture.rgba.size(); offset += 4u) {
			const float x = (static_cast<float>(texture.rgba[offset]) / 255.0f) * 2.0f - 1.0f;
			const float y = (static_cast<float>(texture.rgba[offset + 1u]) / 255.0f) * 2.0f - 1.0f;
			const float slope = std::clamp(std::sqrt(x * x + y * y), 0.0f, 1.0f);
			const auto bin = static_cast<std::size_t>(std::lround(slope * 255.0f));
			++histogram[std::min(bin, histogram.size() - 1u)];
			++samples;
		}
		const std::size_t target = static_cast<std::size_t>(std::ceil(static_cast<float>(samples) * 0.95f));
		std::size_t cumulative = 0u;
		for (std::size_t bin = 0; bin < histogram.size(); ++bin) {
			cumulative += histogram[bin];
			if (cumulative >= target)
				return static_cast<float>(bin) / 255.0f;
		}
		return 0.0f;
	}
}

TEST_CASE("spaceship catalog exposes the complete unit fleet", "[unit][gen_model]")
{
	const auto ships = catalog();
	REQUIRE(ships.size() == 6);
	REQUIRE(ships[0].id == "basic");
	REQUIRE(ships[1].id == "elite");
	REQUIRE(ships[2].id == "fastElite");
	REQUIRE(ships[3].id == "mothership");
	REQUIRE(ships[4].id == "player");
	REQUIRE(ships[5].id == "terminator");
	REQUIRE(ships[0].mounts.size() == 2);
	REQUIRE(ships[1].mounts.size() == 2);
	REQUIRE(ships[2].mounts.size() == 2);
	REQUIRE(ships[3].mounts.size() == 8);
	REQUIRE(ships[4].mounts.size() == 4);
	REQUIRE(ships[5].mounts.size() == 32);
	REQUIRE(ships[2].mounts[0].traverseHalfAngleDegrees == Catch::Approx(5.0f));
	REQUIRE(ships[2].mounts[1].traverseHalfAngleDegrees == Catch::Approx(5.0f));
	REQUIRE(ships[4].mountAttachment.directBlisterGapScale == Catch::Approx(2.0f));
	REQUIRE(ships[3].mountAttachment.directBlisterGapScale == Catch::Approx(4.2f));
	REQUIRE(ships[5].mountAttachment.directBlisterGapScale == Catch::Approx(4.2f));
	for (std::size_t left = 0; left < ships.size(); ++left)
		for (std::size_t right = left + 1u; right < ships.size(); ++right) {
			CAPTURE(ships[left].id, ships[right].id);
			REQUIRE_FALSE(ships[left].material == ships[right].material);
		}
}

TEST_CASE("automatic mount capabilities are independent of runtime projections", "[unit][gen_model]")
{
	std::ifstream source("assets/config/spaceships.json");
	REQUIRE(source.good());
	nlohmann::json root;
	source >> root;
	auto& player = root["ships"]["player"];
	auto& weaponLayout = player["design"]["weaponLayout"];
	REQUIRE(weaponLayout["capabilities"].is_array());
	REQUIRE(weaponLayout["capabilities"].size() == player["mounts"].size());

	const auto loadPlayer = [](const nlohmann::json& catalogJson) {
		std::istringstream input(catalogJson.dump());
		for (const Settings& settings : gen_model::spaceship::loadCatalog(input))
			if (settings.id == "player")
				return settings;
		throw std::logic_error("player profile is missing from spaceship catalog");
	};
	const Settings baseline = loadPlayer(root);
	player["mounts"][0]["position"]["x"] = 999.0f;
	player["mounts"][0]["supportRoot"]["x"] = 998.0f;
	player["mounts"][0]["supportWidth"] = 9.0f;
	const Settings changedProjection = loadPlayer(root);
	REQUIRE(gen_model::spaceship::fingerprint(changedProjection)
		== gen_model::spaceship::fingerprint(baseline));

	weaponLayout["capabilities"][0]["supportWidth"] = 0.61f;
	const Settings changedCapability = loadPlayer(root);
	REQUIRE(gen_model::spaceship::fingerprint(changedCapability)
		!= gen_model::spaceship::fingerprint(baseline));
}

TEST_CASE("automatic mount facing is fixed forward except for explicit terminator nulls", "[unit][gen_model]")
{
	const auto ships = catalog();
	for (const Settings& settings : ships) {
		for (const auto& mount : settings.mounts) {
			CAPTURE(settings.id, mount.id);
			if (settings.id == "terminator") {
				REQUIRE_FALSE(mount.requestedFacing.has_value());
				continue;
			}
			REQUIRE(mount.requestedFacing.has_value());
			REQUIRE(mount.requestedFacing->x == Catch::Approx(0.0f));
			REQUIRE(mount.requestedFacing->y == Catch::Approx(0.0f));
			REQUIRE(mount.requestedFacing->z == Catch::Approx(1.0f));
		}
	}

	std::ifstream source("assets/config/spaceships.json");
	REQUIRE(source.good());
	nlohmann::json root;
	source >> root;
	root["ships"]["player"]["design"]["weaponLayout"]["capabilities"][0]["facingDirection"] =
		{{"x", 1.0f}, {"y", 0.0f}, {"z", 1.0f}};
	std::istringstream input(root.dump());
	Settings changed;
	for (const Settings& candidate : gen_model::spaceship::loadCatalog(input)) {
		if (candidate.id == "player") {
			changed = candidate;
			break;
		}
	}
	REQUIRE(changed.mounts[0].requestedFacing.has_value());
	REQUIRE(changed.mounts[0].requestedFacing->x == Catch::Approx(0.7071067f).margin(0.0001f));
	REQUIRE(changed.mounts[0].requestedFacing->z == Catch::Approx(0.7071067f).margin(0.0001f));
	REQUIRE(gen_model::spaceship::fingerprint(changed) != gen_model::spaceship::fingerprint(catalog()[4]));

	root["ships"]["player"]["design"]["weaponLayout"]["capabilities"][0]["facingDirection"] = nullptr;
	std::istringstream nullInput(root.dump());
	Settings freeFacing;
	for (const Settings& candidate : gen_model::spaceship::loadCatalog(nullInput)) {
		if (candidate.id == "player") {
			freeFacing = candidate;
			break;
		}
	}
	REQUIRE_FALSE(freeFacing.mounts[0].requestedFacing.has_value());
}

TEST_CASE("unit parent collision clearance uses the authored radius-one contract", "[unit][gen_model]")
{
	gen_model::spaceship::MountSettings mount;
	mount.turretRadius = 0.25f;
	mount.position = {1.27f, 0.0f, 0.0f};
	CHECK(gen_model::spaceship::weapon_layout::unitParentCollisionClearance(mount)
		== Catch::Approx(0.02f).margin(0.00001f));
	mount.position = {1.10f, 0.0f, 0.0f};
	CHECK(gen_model::spaceship::weapon_layout::unitParentCollisionClearance(mount)
		== Catch::Approx(-0.15f).margin(0.00001f));
}

TEST_CASE("system details anchor to the emitted structural skin", "[unit][gen_model]")
{
	gen_model::spaceship::detail::MeshBuilder builder;
	builder.addBox(
		{0.0f, 0.0f, 0.0f},
		{2.0f, 2.0f, 2.0f},
		gen_model::spaceship::detail::Surface::Armor
	);
	REQUIRE(builder.topStructuralSurfaceY(0.0f, 0.0f).value() == Catch::Approx(1.0f));
	REQUIRE_FALSE(builder.topStructuralSurfaceY(1.5f, 1.5f).has_value());
	builder.addBox(
		{0.0f, 3.0f, 0.0f},
		{1.0f, 1.0f, 1.0f},
		gen_model::spaceship::detail::Surface::Canopy
	);
	REQUIRE(builder.topStructuralSurfaceY(0.0f, 0.0f).value() == Catch::Approx(1.0f));
}

TEST_CASE("spaceship mesh finish welds positions and removes duplicate faces", "[unit][gen_model]")
{
	gen_model::spaceship::detail::MeshBuilder builder;
	builder.addTriangle(
		{0.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		gen_model::spaceship::detail::Surface::Armor
	);
	builder.addTriangle(
		{0.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		gen_model::spaceship::detail::Surface::Armor
	);
	const auto finished = builder.finish();
	REQUIRE(finished.mesh.triangles.size() == 1u);
	REQUIRE(finished.mesh.positions.size() == 3u);
	REQUIRE(finished.mesh.texcoords.size() >= 3u);
	REQUIRE(finished.mesh.normals.size() >= 3u);
}

TEST_CASE("spaceship mesh finish removes a fully buried render shell", "[unit][gen_model]")
{
	gen_model::spaceship::detail::MeshBuilder builder;
	builder.addBox(
		{0.0f, 0.0f, 0.0f},
		{4.0f, 4.0f, 4.0f},
		gen_model::spaceship::detail::Surface::Armor
	);
	builder.addBox(
		{0.0f, 0.0f, 0.0f},
		{2.0f, 2.0f, 2.0f},
		gen_model::spaceship::detail::Surface::Structure
	);

	const auto finished = builder.finish();
	REQUIRE(finished.mesh.triangles.size() == 12u);
	REQUIRE(finished.mesh.positions.size() == 8u);
}

TEST_CASE("spaceship mesh finish rejects partially overlapping coplanar shells", "[unit][gen_model]")
{
	gen_model::spaceship::detail::MeshBuilder builder;
	builder.addBox(
		{-0.35f, 0.0f, 0.0f},
		{2.0f, 2.0f, 2.0f},
		gen_model::spaceship::detail::Surface::Armor
	);
	builder.addBox(
		{0.35f, 0.0f, 0.0f},
		{2.0f, 2.0f, 2.0f},
		gen_model::spaceship::detail::Surface::Structure
	);

	REQUIRE_THROWS_WITH(
		builder.finish(),
		Catch::Matchers::ContainsSubstring("coplanar overlapping faces")
	);
}

TEST_CASE("spaceship generator is deterministic and structurally reports mounts", "[unit][gen_model]")
{
	const Settings settings = reducedPlayerSettings();
	const auto first = gen_model::spaceship::generate(settings);
	const auto second = gen_model::spaceship::generate(settings);

	REQUIRE(first.settingsFingerprint == second.settingsFingerprint);
	REQUIRE(first.asset.mesh.positions == second.asset.mesh.positions);
	REQUIRE(first.asset.mesh.texcoords == second.asset.mesh.texcoords);
	REQUIRE(first.asset.mesh.normals == second.asset.mesh.normals);
	REQUIRE(first.asset.mesh.triangles == second.asset.mesh.triangles);
	REQUIRE(first.asset.texture.rgba == second.asset.texture.rgba);
	REQUIRE(first.asset.normalMap.rgba == second.asset.normalMap.rgba);
	REQUIRE(first.mounts.size() == settings.mounts.size());

	bool variedAlbedo = false;
	for (std::size_t offset = 4; offset < first.asset.texture.rgba.size(); offset += 4) {
		if (first.asset.texture.rgba[offset] != first.asset.texture.rgba[0]
			|| first.asset.texture.rgba[offset + 1] != first.asset.texture.rgba[1]
			|| first.asset.texture.rgba[offset + 2] != first.asset.texture.rgba[2]) {
			variedAlbedo = true;
			break;
		}
	}
	REQUIRE(variedAlbedo);

	for (const auto& report : first.mounts) {
		REQUIRE(report.structurallyConnected);
		REQUIRE(report.supportThickness >= 0.375f);
		REQUIRE(report.minimumClearance >= 0.02f);
	}
	for (const auto& mount : first.resolvedMounts)
		REQUIRE(gen_model::spaceship::weapon_layout::unitParentCollisionClearance(mount) >= 0.02f);
	for (const auto& triangle : first.asset.mesh.triangles) {
		for (const int index : triangle.positionIndices)
			REQUIRE(index >= 0);
		for (const int index : triangle.texcoordIndices)
			REQUIRE(index >= 0);
		for (const int index : triangle.normalIndices) {
			REQUIRE(index >= 0);
			REQUIRE(index < static_cast<int>(first.asset.mesh.normals.size()));
			REQUIRE(std::isfinite(length(first.asset.mesh.normals[static_cast<std::size_t>(index)])));
			REQUIRE(length(first.asset.mesh.normals[static_cast<std::size_t>(index)]) == Catch::Approx(1.0f).margin(0.001f));
		}
	}
}

TEST_CASE("spaceship normal relief is visible and texture-resolution invariant", "[unit][gen_model]")
{
	Settings lowSettings = reducedPlayerSettings();
	lowSettings.textureWidth = 64;
	lowSettings.textureHeight = 64;
	Settings highSettings = lowSettings;
	highSettings.textureWidth = 256;
	highSettings.textureHeight = 256;
	const auto low = gen_model::spaceship::generate(lowSettings);
	const auto high = gen_model::spaceship::generate(highSettings);
	const float lowP95 = normalSlopeP95(low.asset.normalMap);
	const float highP95 = normalSlopeP95(high.asset.normalMap);
	CAPTURE(lowP95, highP95);
	REQUIRE(lowP95 >= 0.08f);
	REQUIRE(highP95 >= 0.08f);
	REQUIRE(highP95 == Catch::Approx(lowP95).margin(0.035f));
}

TEST_CASE("generated ships visibly express their planned support systems", "[unit][gen_model]")
{
	for (Settings settings : catalog()) {
		settings.textureWidth = 32;
		settings.textureHeight = 32;
		const auto generated = gen_model::spaceship::generate(settings);
		CAPTURE(settings.id);
		REQUIRE(generated.systemDetails.fuelHousings >= generated.resolvedEngines.size());
		REQUIRE(generated.systemDetails.feedTrunks >= generated.resolvedEngines.size());
		REQUIRE(generated.systemDetails.reactorShields >= 1u);
		REQUIRE(generated.systemDetails.radiatorPanels >= 2u);
		REQUIRE(generated.systemDetails.serviceAccessPanels >= 1u);
	}
}

TEST_CASE("sparse fighter mounts grow directly from the wing skin", "[unit][gen_model]")
{
	for (Settings settings : catalog()) {
		if (!isFighter(settings))
			continue;
		settings.textureWidth = 32;
		settings.textureHeight = 32;
		const auto generated = gen_model::spaceship::generate(settings);
		for (const auto& report : generated.mounts) {
			CAPTURE(settings.id, report.id, report.supportLength, report.takeoffAngleDegrees);
			REQUIRE(report.directBlister);
		}
	}
}

TEST_CASE("fighter pressure skin top strips face outward", "[unit][gen_model]")
{
	const auto generated = gen_model::spaceship::generate(reducedPlayerSettings());
	constexpr std::size_t airframeIntervals = 32;
	constexpr std::size_t crossSectionStrips = 24;
	constexpr std::size_t centerStrips[] = {11, 12};
	constexpr std::size_t trianglesPerStripPair = 4;
	REQUIRE(generated.asset.mesh.triangles.size() >= airframeIntervals * crossSectionStrips * trianglesPerStripPair);

	for (std::size_t station = 0; station < airframeIntervals; ++station) {
		for (const std::size_t strip : centerStrips) {
			const std::size_t firstTriangle = (
				station * crossSectionStrips + strip
			) * trianglesPerStripPair;
			for (std::size_t triangleOffset = 0; triangleOffset < 2; ++triangleOffset) {
				const auto& triangle = generated.asset.mesh.triangles[firstTriangle + triangleOffset];
				REQUIRE(faceNormal(generated.asset.mesh, triangle).y >= 0.0f);
			}
		}
	}
}

TEST_CASE("fighter pressure hull end caps face out of the authored hull", "[unit][gen_model]")
{
	for (Settings settings : catalog()) {
		if (!isFighter(settings))
			continue;
		settings.textureWidth = 32;
		settings.textureHeight = 32;
		const auto generated = gen_model::spaceship::generate(settings);
		const float frontZ = settings.hull.length * 0.5f;
		const float rearZ = -frontZ;
		std::size_t frontFaces = 0;
		std::size_t rearFaces = 0;
		for (const auto& triangle : generated.asset.mesh.triangles) {
			const auto& a = generated.asset.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[0])];
			const auto& b = generated.asset.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[1])];
			const auto& c = generated.asset.mesh.positions[static_cast<std::size_t>(triangle.positionIndices[2])];
			const Point3 normal = faceNormal(generated.asset.mesh, triangle);
			if (std::abs(a.z - frontZ) < 0.0001f
				&& std::abs(b.z - frontZ) < 0.0001f
				&& std::abs(c.z - frontZ) < 0.0001f) {
				INFO("fighter=" << settings.id << " cap=front");
				REQUIRE(normal.z > 0.0f);
				++frontFaces;
			}
			if (std::abs(a.z - rearZ) < 0.0001f
				&& std::abs(b.z - rearZ) < 0.0001f
				&& std::abs(c.z - rearZ) < 0.0001f) {
				INFO("fighter=" << settings.id << " cap=rear");
				REQUIRE(normal.z < 0.0f);
				++rearFaces;
			}
		}
		REQUIRE(frontFaces > 0);
		REQUIRE(rearFaces > 0);
	}
}

TEST_CASE("spaceship topology audit rejects open and inconsistently wound shells", "[unit][gen_model]")
{
	const auto closed = gen_model::spaceship::topology::auditClosedOrientedMesh(tetrahedron());
	REQUIRE(closed.closedAndOriented());

	auto open = tetrahedron();
	open.triangles.pop_back();
	const auto openReport = gen_model::spaceship::topology::auditClosedOrientedMesh(open);
	REQUIRE(openReport.boundaryEdges == 3);
	REQUIRE_FALSE(openReport.closedAndOriented());
	REQUIRE_THROWS_AS(
		gen_model::spaceship::topology::requireClosedOrientedMesh(open),
		std::invalid_argument
	);

	auto reversed = tetrahedron();
	std::swap(
		reversed.triangles.front().positionIndices[1],
		reversed.triangles.front().positionIndices[2]
	);
	const auto reversedReport = gen_model::spaceship::topology::auditClosedOrientedMesh(reversed);
	REQUIRE(reversedReport.inconsistentWindingEdges == 3);
	REQUIRE_FALSE(reversedReport.closedAndOriented());

	auto insideOut = tetrahedron();
	for (auto& triangle : insideOut.triangles)
		std::swap(triangle.positionIndices[1], triangle.positionIndices[2]);
	const auto insideOutReport = gen_model::spaceship::topology::auditClosedOrientedMesh(
		insideOut
	);
	REQUIRE(insideOutReport.inconsistentWindingEdges == 0);
	REQUIRE(insideOutReport.nonPositiveVolumeComponents == 1);
	REQUIRE_FALSE(insideOutReport.closedAndOriented());
}

TEST_CASE("fighter wing edge closures face the correct travel directions", "[unit][gen_model]")
{
	Settings settings;
	for (const Settings& candidate : catalog()) {
		if (candidate.id == "basic") {
			settings = candidate;
			break;
		}
	}
	REQUIRE_FALSE(settings.id.empty());
	settings.textureWidth = 32;
	settings.textureHeight = 32;
	const auto generated = gen_model::spaceship::generate(settings);
	// The integrated shell has 3,200 longitudinal triangles plus two complete
	// 50-edge cap fans. Each wing half has 144 loft triangles plus two 12-edge caps.
	constexpr std::size_t integratedFighterTriangles = 3300;
	constexpr std::size_t wingTrianglesPerHalf = 168;
	constexpr std::size_t wingStationIntervals = 6;
	constexpr std::size_t wingSections = 12;
	constexpr std::size_t trianglesPerSection = 2;

	for (std::size_t side = 0; side < 2; ++side) {
		for (std::size_t station = 0; station < wingStationIntervals; ++station) {
			const std::size_t sectionStart = integratedFighterTriangles
				+ side * wingTrianglesPerHalf
				+ station * wingSections * trianglesPerSection;
			for (std::size_t triangleOffset = 0; triangleOffset < trianglesPerSection; ++triangleOffset) {
				const auto& leadingShoulderTriangle = generated.asset.mesh.triangles[
					sectionStart + triangleOffset
				];
				const auto& leadingClosureTriangle = generated.asset.mesh.triangles[
					sectionStart + 11u * trianglesPerSection + triangleOffset
				];
				const auto& trailingTriangle = generated.asset.mesh.triangles[
					sectionStart + 5u * trianglesPerSection + triangleOffset
				];
				REQUIRE(faceNormal(generated.asset.mesh, leadingShoulderTriangle).z > 0.0f);
				REQUIRE(faceNormal(generated.asset.mesh, leadingClosureTriangle).z > 0.0f);
				REQUIRE(faceNormal(generated.asset.mesh, trailingTriangle).z < 0.0f);
			}
		}
	}
}

TEST_CASE("spaceship generator rejects unsafe mount and texture settings", "[unit][gen_model]")
{
	SECTION("barrel clearance contract") {
		Settings settings = reducedPlayerSettings();
		settings.mounts.front().barrelLength = settings.mounts.front().turretRadius * 10.0f;
		REQUIRE_THROWS_AS(gen_model::spaceship::generate(settings), std::invalid_argument);
	}

	SECTION("texture bounds") {
		Settings settings = reducedPlayerSettings();
		settings.textureWidth = 1;
		REQUIRE_THROWS_AS(gen_model::spaceship::generate(settings), std::invalid_argument);
	}

	SECTION("catalog schema") {
		std::istringstream invalidSchema(R"({"schemaVersion":2,"defaults":{},"ships":{}})");
		REQUIRE_THROWS_AS(gen_model::spaceship::loadCatalog(invalidSchema), std::invalid_argument);
	}

	SECTION("material relief") {
		Settings settings = reducedPlayerSettings();
		settings.material.normalStrength = 0.0f;
		REQUIRE_THROWS_AS(gen_model::spaceship::generate(settings), std::invalid_argument);
	}
}

TEST_CASE("every fleet profile generates with reduced textures", "[unit][gen_model]")
{
	for (Settings settings : catalog()) {
		settings.textureWidth = 32;
		settings.textureHeight = 32;
		const auto generated = gen_model::spaceship::generate(settings);
		CAPTURE(settings.id);
		REQUIRE(generated.asset.mesh.triangles.size() > 0);
		REQUIRE(generated.asset.texture.rgba.size() == 32u * 32u * 4u);
		REQUIRE(generated.asset.normalMap.rgba.size() == 32u * 32u * 4u);
		REQUIRE(generated.mounts.size() == settings.mounts.size());
		for (std::size_t index = 0; index < generated.resolvedMounts.size(); ++index) {
			const auto& mount = generated.resolvedMounts[index];
			CAPTURE(settings.id, index);
			if (settings.id != "terminator") {
				REQUIRE(mount.requestedFacing.has_value());
				REQUIRE(gen_model::gen_types::dot(
					gen_model::gen_types::normalize(*mount.requestedFacing),
					gen_model::gen_types::normalize(mount.forward)
				) == Catch::Approx(1.0f).margin(0.0001f));
			}
			REQUIRE(gen_model::spaceship::weapon_layout::unitParentCollisionClearance(mount) >= 0.02f);
		}
		REQUIRE(generated.materialDetails.normalSlopeP95 >= 0.06f);
		REQUIRE(generated.materialDetails.secondaryCoverage >= 0.05f);
		REQUIRE(generated.materialDetails.secondaryCoverage <= 0.75f);
		REQUIRE(generated.materialDetails.accentCoverage >= 0.01f);
		REQUIRE(generated.materialDetails.accentCoverage <= 0.30f);
		REQUIRE(generated.materialDetails.thermalCoverage >= 0.05f);
		REQUIRE(generated.materialDetails.thermalCoverage <= 0.95f);
		const auto topology = gen_model::spaceship::topology::auditClosedOrientedMesh(
			generated.asset.mesh
		);
		INFO("spaceship=" << settings.id);
		REQUIRE(topology.invalidTriangles == 0);
		REQUIRE(topology.degenerateTriangles == 0);
		REQUIRE(topology.boundaryEdges == 0);
		REQUIRE(topology.nonManifoldEdges == 0);
		REQUIRE(topology.inconsistentWindingEdges == 0);
		REQUIRE(topology.nonPositiveVolumeComponents == 0);
	}
}

TEST_CASE("dense batteries keep mirrored attachment evidence", "[unit][gen_model]")
{
	Settings settings;
	for (const Settings& candidate : catalog()) {
		if (candidate.id == "terminator") {
			settings = candidate;
			break;
		}
	}
	REQUIRE_FALSE(settings.id.empty());
	settings.textureWidth = 32;
	settings.textureHeight = 32;
	const auto generated = gen_model::spaceship::generate(settings);
	REQUIRE(generated.mounts.size() == settings.mounts.size());
	REQUIRE(generated.mounts.size() == generated.resolvedMounts.size());
	for (std::size_t index = 0; index < generated.mounts.size(); ++index) {
		const auto& attachment = generated.mounts[index].attachmentPoint;
		const auto& railDeck = generated.resolvedMounts[index].supportRoot;
		CAPTURE(index, generated.mounts[index].id);
		REQUIRE(attachment.x == Catch::Approx(railDeck.x).margin(0.001f));
		REQUIRE(attachment.y == Catch::Approx(railDeck.y).margin(0.001f));
		REQUIRE(attachment.z == Catch::Approx(railDeck.z).margin(0.001f));
	}
	REQUIRE(generated.mounts.size() % 2u == 0u);
	for (std::size_t index = 0; index < generated.mounts.size(); index += 2u) {
		const auto& left = generated.mounts[index];
		const auto& right = generated.mounts[index + 1u];
		CAPTURE(index, left.id, right.id);
		REQUIRE(left.attachmentPoint.x == Catch::Approx(-right.attachmentPoint.x).margin(0.001f));
		REQUIRE(left.attachmentPoint.y == Catch::Approx(right.attachmentPoint.y).margin(0.001f));
		REQUIRE(left.attachmentPoint.z == Catch::Approx(right.attachmentPoint.z).margin(0.001f));
		REQUIRE(left.takeoffAngleDegrees == Catch::Approx(right.takeoffAngleDegrees).margin(0.001f));
		REQUIRE(left.supportLength == Catch::Approx(right.supportLength).margin(0.001f));
		REQUIRE(left.structurallyConnected);
		REQUIRE(right.structurallyConnected);
	}
}

TEST_CASE("mount attachment policy rejects degenerate tradeoff weights", "[unit][gen_model]")
{
	Settings settings = reducedPlayerSettings();
	settings.mountAttachment.distanceWeight = 0.0f;
	settings.mountAttachment.angleWeight = 0.0f;
	REQUIRE_THROWS_AS(gen_model::spaceship::generate(settings), std::invalid_argument);

	settings = reducedPlayerSettings();
	settings.mountAttachment.minimumTakeoffAngleDegrees = 45.0f;
	settings.mountAttachment.preferredTakeoffAngleDegrees = 30.0f;
	REQUIRE_THROWS_AS(gen_model::spaceship::generate(settings), std::invalid_argument);
}
