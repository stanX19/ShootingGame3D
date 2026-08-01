#include "catch2/catch_amalgamated.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "gen_model/asteroid_generator.hpp"
#include "gen_model/asteroid_big.hpp"
#include "gen_model/asteroid_small.hpp"

namespace {
	float radius(const gen_model::gen_types::Point3& point) {
		return std::sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
	}
}

TEST_CASE("asteroid generator produces deterministic low-poly assets", "[unit][gen_model]")
{
	const gen_model::asteroid::Settings settings{};
	auto generationSettings = settings;
	generationSettings.textureWidth = 64;
	generationSettings.textureHeight = 32;
	const auto first = gen_model::asteroid::generate(generationSettings);
	const auto second = gen_model::asteroid::generate(generationSettings);

	REQUIRE(first.mesh.triangles.size() == 1656);
	REQUIRE(first.mesh.positions.size() == static_cast<std::size_t>((settings.latitudeSegments + 1) * (settings.longitudeSegments + 1)));
	REQUIRE(first.mesh.texcoords.size() == first.mesh.positions.size());
        REQUIRE(first.mesh.normals.size() == first.mesh.positions.size());
	REQUIRE(first.mesh.positions == second.mesh.positions);
	REQUIRE(first.mesh.texcoords == second.mesh.texcoords);
	REQUIRE(first.mesh.normals == second.mesh.normals);
	REQUIRE(first.mesh.triangles == second.mesh.triangles);

	float minimumRadius = 2.0f;
	float maximumRadius = 0.0f;
	for (const auto& point : first.mesh.positions) {
		const float pointRadius = radius(point);
		minimumRadius = std::min(minimumRadius, pointRadius);
		maximumRadius = std::max(maximumRadius, pointRadius);
		REQUIRE(pointRadius >= 0.95f);
		REQUIRE(pointRadius <= 1.05f);
	}
	REQUIRE(minimumRadius == Catch::Approx(0.95f).margin(0.0001f));
	REQUIRE(maximumRadius == Catch::Approx(1.05f).margin(0.0001f));
	for (const auto& triangle : first.mesh.triangles) {
		const auto& a = first.mesh.positions[triangle.positionIndices[0]];
		const auto& b = first.mesh.positions[triangle.positionIndices[1]];
		const auto& c = first.mesh.positions[triangle.positionIndices[2]];
                const auto center = gen_model::gen_types::Point3{
			(a.x + b.x + c.x) / 3.0f,
			(a.y + b.y + c.y) / 3.0f,
			(a.z + b.z + c.z) / 3.0f
		};
                for (const int normalIndex : triangle.normalIndices) {
                        REQUIRE(normalIndex >= 0);
                        REQUIRE(normalIndex < static_cast<int>(first.mesh.normals.size()));
                        const auto& normal = first.mesh.normals[static_cast<std::size_t>(normalIndex)];
                        REQUIRE(std::abs(radius(normal) - 1.0f) < 0.001f);
                        REQUIRE(normal.x * center.x + normal.y * center.y + normal.z * center.z >= 0.0f);
                }
	}
}

TEST_CASE("asteroid texture is varied and longitude-seam safe", "[unit][gen_model]")
{
	const auto defaultSettings = gen_model::asteroid::Settings{};
	REQUIRE(defaultSettings.textureWidth == 2048);
	REQUIRE(defaultSettings.textureHeight == 1024);
	auto settings = defaultSettings;
	settings.textureWidth = 64;
	settings.textureHeight = 32;
	const auto texture = gen_model::asteroid::generate(settings).texture;
	REQUIRE(texture.width == settings.textureWidth);
	REQUIRE(texture.height == settings.textureHeight);
	REQUIRE(texture.rgba.size() == static_cast<std::size_t>(texture.width * texture.height * 4));

	bool hasVariation = false;
	for (std::size_t offset = 4; offset < texture.rgba.size(); offset += 4) {
		if (texture.rgba[offset] != texture.rgba[0]) {
			hasVariation = true;
			break;
		}
	}
	REQUIRE(hasVariation);

	for (int row = 0; row < texture.height; ++row) {
		const std::size_t left = static_cast<std::size_t>(row * texture.width * 4);
		const std::size_t right = static_cast<std::size_t>((row * texture.width + texture.width - 1) * 4);
		for (int channel = 0; channel < 4; ++channel) {
			REQUIRE(texture.rgba[left + channel] == texture.rgba[right + channel]);
		}
	}
}

TEST_CASE("asteroid mesh shares normals at adjacent triangle corners", "[unit][gen_model]")
{
	auto settings = gen_model::asteroid::Settings{};
	settings.textureWidth = 8;
	settings.textureHeight = 4;
	const auto asset = gen_model::asteroid::generate(settings);
	std::vector<int> firstNormalByPosition(asset.mesh.positions.size(), -1);
	bool foundSharedPosition = false;

        for (const auto& triangle : asset.mesh.triangles) {
                for (std::size_t corner = 0; corner < triangle.positionIndices.size(); ++corner) {
                        const int positionIndex = triangle.positionIndices[corner];
                        int& firstNormalIndex = firstNormalByPosition[static_cast<std::size_t>(positionIndex)];
                        if (firstNormalIndex < 0) {
                                firstNormalIndex = triangle.normalIndices[corner];
                                continue;
                        }
                        foundSharedPosition = true;
                        REQUIRE(triangle.normalIndices[corner] == firstNormalIndex);
                }
        }

        REQUIRE(foundSharedPosition);
        const int rowWidth = settings.longitudeSegments + 1;
        for (int row = 0; row <= settings.latitudeSegments; ++row) {
                const int seamPosition = row * rowWidth + settings.longitudeSegments;
                const int firstPosition = row * rowWidth;
                if (firstNormalByPosition[static_cast<std::size_t>(seamPosition)] < 0 || firstNormalByPosition[static_cast<std::size_t>(firstPosition)] < 0) {
                        continue;
                }
                REQUIRE(firstNormalByPosition[static_cast<std::size_t>(seamPosition)] == firstNormalByPosition[static_cast<std::size_t>(firstPosition)]);
        }
}

TEST_CASE("asteroid normal map is deterministic, varied, and seam-safe", "[unit][gen_model]")
{
	auto settings = gen_model::asteroid::Settings{};
	settings.textureWidth = 64;
	settings.textureHeight = 32;
	const auto first = gen_model::asteroid::generate(settings);
	const auto second = gen_model::asteroid::generate(settings);
	const auto& normalMap = first.normalMap;

	REQUIRE(normalMap.width == settings.textureWidth);
	REQUIRE(normalMap.height == settings.textureHeight);
	REQUIRE(normalMap.rgba.size() == static_cast<std::size_t>(normalMap.width * normalMap.height * 4));
	REQUIRE(normalMap.rgba == second.normalMap.rgba);

	bool hasVariation = false;
	bool hasStrongNormalVariation = false;
	for (std::size_t offset = 4; offset < normalMap.rgba.size(); offset += 4) {
		if (normalMap.rgba[offset] != normalMap.rgba[0] ||
			normalMap.rgba[offset + 1] != normalMap.rgba[1] ||
			normalMap.rgba[offset + 2] != normalMap.rgba[2]) {
			hasVariation = true;
		}
		if (normalMap.rgba[offset + 2] < 210)
			hasStrongNormalVariation = true;
	}
	REQUIRE(hasVariation);
	REQUIRE(hasStrongNormalVariation);

	for (int row = 0; row < normalMap.height; ++row) {
		const std::size_t left = static_cast<std::size_t>(row * normalMap.width * 4);
		const std::size_t right = static_cast<std::size_t>((row * normalMap.width + normalMap.width - 1) * 4);
		for (int channel = 0; channel < 4; ++channel) {
			REQUIRE(normalMap.rgba[left + channel] == normalMap.rgba[right + channel]);
		}
	}
}

TEST_CASE("asteroid generator rejects invalid settings", "[unit][gen_model]")
{
	SECTION("latitude segments") {
		auto settings = gen_model::asteroid::Settings{};
		settings.latitudeSegments = 1;
		REQUIRE_THROWS_AS(gen_model::asteroid::generate(settings), std::invalid_argument);
	}

	SECTION("minimum radius") {
		auto settings = gen_model::asteroid::Settings{};
		settings.minRadius = 0.0f;
		REQUIRE_THROWS_AS(gen_model::asteroid::generate(settings), std::invalid_argument);
	}

	SECTION("radius ordering") {
		auto settings = gen_model::asteroid::Settings{};
		settings.maxRadius = 0.9f;
		REQUIRE_THROWS_AS(gen_model::asteroid::generate(settings), std::invalid_argument);
	}

	SECTION("texture dimensions") {
		auto settings = gen_model::asteroid::Settings{};
		settings.textureWidth = 1;
		REQUIRE_THROWS_AS(gen_model::asteroid::generate(settings), std::invalid_argument);
	}
}

TEST_CASE("asteroid profiles share the generator with explicit topology budgets", "[unit][gen_model]")
{
	const auto small = gen_model::asteroid::small::settings();
	const auto big = gen_model::asteroid::big::settings();

	REQUIRE_FALSE(small.useMacroGeometry);
	REQUIRE(big.useMacroGeometry);
	REQUIRE(small.macroCraters.count == 0);
	REQUIRE(small.mediumCraters.count == 0);
	REQUIRE(small.fineCraters.count == 64);
	REQUIRE(big.macroCraters.count == 10);
	REQUIRE(big.mediumCraters.count == 36);
	REQUIRE(big.fineCraters.count == 80);
	REQUIRE(big.broadFeatureStrength > small.broadFeatureStrength);
	REQUIRE(big.grainFeatureStrength < small.grainFeatureStrength);
	REQUIRE(small.normalMapStrength == Catch::Approx(12.0f));
	REQUIRE(big.normalMapStrength == Catch::Approx(64.0f));
	REQUIRE(small.latitudeSegments == 24);
	REQUIRE(small.longitudeSegments == 36);
	REQUIRE(big.latitudeSegments == 32);
	REQUIRE(big.longitudeSegments == 48);
	REQUIRE(2 * small.longitudeSegments * (small.latitudeSegments - 1) == 1656);
	REQUIRE(2 * big.longitudeSegments * (big.latitudeSegments - 1) == 2976);
	REQUIRE(small.minRadius == Catch::Approx(0.95f));
	REQUIRE(small.maxRadius == Catch::Approx(1.05f));
	REQUIRE(big.minRadius == Catch::Approx(0.95f));
	REQUIRE(big.maxRadius == Catch::Approx(1.05f));

	auto generatedBigSettings = big;
	generatedBigSettings.textureWidth = 64;
	generatedBigSettings.textureHeight = 32;
	const auto generatedBig = gen_model::asteroid::generate(generatedBigSettings);
	REQUIRE(generatedBig.mesh.triangles.size() == 2976);
	float minimumBigRadius = 2.0f;
	float maximumBigRadius = 0.0f;
	for (const auto& point : generatedBig.mesh.positions) {
		const float pointRadius = radius(point);
		minimumBigRadius = std::min(minimumBigRadius, pointRadius);
		maximumBigRadius = std::max(maximumBigRadius, pointRadius);
	}
	REQUIRE(minimumBigRadius == Catch::Approx(0.95f).margin(0.0001f));
	REQUIRE(maximumBigRadius == Catch::Approx(1.05f).margin(0.0001f));
	std::uint8_t minimumRed = 255;
	std::uint8_t maximumRed = 0;
	std::uint8_t minimumBlue = 255;
	std::uint8_t maximumBlue = 0;
	for (std::size_t offset = 0; offset < generatedBig.texture.rgba.size(); offset += 4) {
		minimumRed = std::min(minimumRed, generatedBig.texture.rgba[offset]);
		maximumRed = std::max(maximumRed, generatedBig.texture.rgba[offset]);
		minimumBlue = std::min(minimumBlue, generatedBig.texture.rgba[offset + 2]);
		maximumBlue = std::max(maximumBlue, generatedBig.texture.rgba[offset + 2]);
	}
	REQUIRE(maximumRed - minimumRed >= 20);
	REQUIRE(maximumBlue - minimumBlue >= 15);
}

TEST_CASE("asteroid generator accepts the 4096-wide benchmark texture limit", "[unit][gen_model]")
{
	auto settings = gen_model::asteroid::big::settings();
	settings.textureWidth = 4096;
	settings.textureHeight = 2;
	settings.latitudeSegments = 3;
	settings.longitudeSegments = 3;
	const auto asset = gen_model::asteroid::generate(settings);

	REQUIRE(asset.texture.width == 4096);
	REQUIRE(asset.texture.height == 2);
	REQUIRE(asset.normalMap.width == 4096);
	REQUIRE(asset.normalMap.height == 2);
}
