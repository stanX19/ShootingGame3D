#include "catch2/catch_amalgamated.hpp"

#include <cmath>
#include <stdexcept>

#include "gen_model/turret_generator.hpp"

namespace {
	using namespace gen_model::turret;
	using Point3 = gen_model::gen_types::Point3;

	float length(Point3 p) {
		return std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
	}
}

TEST_CASE("Turret generator is deterministic for identical settings", "[model][turret]") {
	SECTION("BasicShooter determinism") {
		const Settings settingsA = defaultBasicShooter();
		const Settings settingsB = defaultBasicShooter();

		const GeneratedTurret a = generate(settingsA);
		const GeneratedTurret b = generate(settingsB);

		REQUIRE(a.asset.mesh.positions.size() == b.asset.mesh.positions.size());
		REQUIRE(a.asset.mesh.triangles.size() == b.asset.mesh.triangles.size());
		REQUIRE(a.asset.texture.rgba == b.asset.texture.rgba);
		REQUIRE(a.asset.normalMap.rgba == b.asset.normalMap.rgba);
		REQUIRE(a.report == b.report);
	}

	SECTION("HeavyMgRifle determinism") {
		const Settings settingsA = defaultHeavyMgRifle();
		const Settings settingsB = defaultHeavyMgRifle();

		const GeneratedTurret a = generate(settingsA);
		const GeneratedTurret b = generate(settingsB);

		REQUIRE(a.asset.mesh.positions.size() == b.asset.mesh.positions.size());
		REQUIRE(a.asset.mesh.triangles.size() == b.asset.mesh.triangles.size());
		REQUIRE(a.asset.texture.rgba == b.asset.texture.rgba);
		REQUIRE(a.asset.normalMap.rgba == b.asset.normalMap.rgba);
		REQUIRE(a.report == b.report);
	}

	SECTION("LazerDeletor determinism") {
		const Settings settingsA = defaultLazerDeletor();
		const Settings settingsB = defaultLazerDeletor();

		const GeneratedTurret a = generate(settingsA);
		const GeneratedTurret b = generate(settingsB);

		REQUIRE(a.asset.mesh.positions.size() == b.asset.mesh.positions.size());
		REQUIRE(a.asset.mesh.triangles.size() == b.asset.mesh.triangles.size());
		REQUIRE(a.asset.texture.rgba == b.asset.texture.rgba);
		REQUIRE(a.asset.normalMap.rgba == b.asset.normalMap.rgba);
		REQUIRE(a.report == b.report);
	}

	SECTION("LazerShotgun determinism") {
		const Settings settingsA = defaultLazerShotgun();
		const Settings settingsB = defaultLazerShotgun();

		const GeneratedTurret a = generate(settingsA);
		const GeneratedTurret b = generate(settingsB);

		REQUIRE(a.asset.mesh.positions.size() == b.asset.mesh.positions.size());
		REQUIRE(a.asset.mesh.triangles.size() == b.asset.mesh.triangles.size());
		REQUIRE(a.asset.texture.rgba == b.asset.texture.rgba);
		REQUIRE(a.asset.normalMap.rgba == b.asset.normalMap.rgba);
		REQUIRE(a.report == b.report);
	}
}

TEST_CASE("Turret geometry respects bounding and topology contracts", "[model][turret]") {
	const std::vector<Settings> profiles = {
		defaultBasicShooter(),
		defaultHeavyMgRifle(),
		defaultLazerDeletor(),
		defaultLazerShotgun()
	};

	for (const auto& settings : profiles) {
		DYNAMIC_SECTION("Profile " << settings.id) {
			const GeneratedTurret turret = generate(settings);
			const auto& mesh = turret.asset.mesh;

			REQUIRE_FALSE(mesh.positions.empty());
			REQUIRE_FALSE(mesh.triangles.empty());
			REQUIRE(mesh.positions.size() == mesh.normals.size());
			REQUIRE(mesh.positions.size() == mesh.texcoords.size());

			// Contract Rule 1 & 2: Center (0, 0, 0) must be center of sphere, radius must be 1.0f
			CHECK(turret.report.pivotPosition.x == 0.0f);
			CHECK(turret.report.pivotPosition.y == 0.0f);
			CHECK(turret.report.pivotPosition.z == 0.0f);
			const std::size_t sphereVertexCount = (10 + 1) * (14 + 1); // 165 vertices in addSphere
			REQUIRE(mesh.positions.size() >= sphereVertexCount);
			for (std::size_t i = 0; i < sphereVertexCount; ++i) {
				const float dist = length(mesh.positions[i]);
				CHECK(std::abs(dist - settings.baseRadius) < 1e-4f);
			}

			// Bounding contract: unit scale with chassis, casing, and barrel
			for (const auto& pos : mesh.positions) {
				CHECK(std::abs(pos.x) <= 1.60f);
				CHECK(pos.y >= -1.15f);
				CHECK(pos.y <= 1.40f);
				CHECK(pos.z >= -2.20f);
				CHECK(pos.z <= 9.00f);
			}

			// Report accuracy
			CHECK(turret.report.boundingRadius <= 9.0f);
			CHECK(turret.report.boundingHeight <= 2.5f);
			CHECK(turret.report.vertexCount == mesh.positions.size());
			CHECK(turret.report.triangleCount == mesh.triangles.size());

			// Topology: valid indices and non-degenerate triangles
			for (const auto& tri : mesh.triangles) {
				for (int corner = 0; corner < 3; ++corner) {
					CHECK(tri.positionIndices[corner] >= 0);
					CHECK(tri.positionIndices[corner] < static_cast<int>(mesh.positions.size()));
				}

				const Point3 p0 = mesh.positions[tri.positionIndices[0]];
				const Point3 p1 = mesh.positions[tri.positionIndices[1]];
				const Point3 p2 = mesh.positions[tri.positionIndices[2]];

				const Point3 v0 = p1 - p0;
				const Point3 v1 = p2 - p0;
				const Point3 c = cross(v0, v1);
				const float area2 = length(c);
				CHECK(area2 > 1e-7f);
			}
		}
	}
}

TEST_CASE("Turret textures have valid dimensions and variation", "[model][turret]") {
	Settings settings = defaultHeavyMgRifle();
	settings.textureWidth = 128;
	settings.textureHeight = 128;

	const GeneratedTurret turret = generate(settings);
	REQUIRE(turret.asset.texture.width == 128);
	REQUIRE(turret.asset.texture.height == 128);
	REQUIRE(turret.asset.texture.rgba.size() == 128 * 128 * 4);

	REQUIRE(turret.asset.normalMap.width == 128);
	REQUIRE(turret.asset.normalMap.height == 128);
	REQUIRE(turret.asset.normalMap.rgba.size() == 128 * 128 * 4);

	// Normal map tangent Z must always be positive (outward from surface)
	for (std::size_t i = 2; i < turret.asset.normalMap.rgba.size(); i += 4) {
		CHECK(turret.asset.normalMap.rgba[i] > 128);
	}
}

TEST_CASE("Turret generator validates input bounds", "[model][turret]") {
	Settings badBase = defaultBasicShooter();
	badBase.baseRadius = -0.5f;
	CHECK_THROWS_AS(generate(badBase), std::invalid_argument);

	Settings badBarrel = defaultHeavyMgRifle();
	badBarrel.barrelLength = 0.0f;
	CHECK_THROWS_AS(generate(badBarrel), std::invalid_argument);
}
