#include "asteroid_generator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {
	constexpr float PI = 3.14159265358979323846f;

	std::uint32_t hashLattice(int x, int y, int z, std::uint32_t seed) {
		std::uint32_t value = seed;
		value ^= static_cast<std::uint32_t>(x) * 0x9E3779B1u;
		value ^= static_cast<std::uint32_t>(y) * 0x85EBCA77u;
		value ^= static_cast<std::uint32_t>(z) * 0xC2B2AE3Du;
		value ^= value >> 16;
		value *= 0x7FEB352Du;
		value ^= value >> 15;
		value *= 0x846CA68Bu;
		return value ^ (value >> 16);
	}

	float latticeValue(int x, int y, int z, std::uint32_t seed) {
		return static_cast<float>(hashLattice(x, y, z, seed)) / 4294967295.0f;
	}

	float smoothStep(float value) {
		return value * value * (3.0f - 2.0f * value);
	}

	float lerp(float left, float right, float amount) {
		return left + (right - left) * amount;
	}

	float valueNoise(gen_model::gen_types::Point3 point, std::uint32_t seed) {
		const int x0 = static_cast<int>(std::floor(point.x));
		const int y0 = static_cast<int>(std::floor(point.y));
		const int z0 = static_cast<int>(std::floor(point.z));
		const float tx = smoothStep(point.x - static_cast<float>(x0));
		const float ty = smoothStep(point.y - static_cast<float>(y0));
		const float tz = smoothStep(point.z - static_cast<float>(z0));

		const float x00 = lerp(latticeValue(x0, y0, z0, seed), latticeValue(x0 + 1, y0, z0, seed), tx);
		const float x10 = lerp(latticeValue(x0, y0 + 1, z0, seed), latticeValue(x0 + 1, y0 + 1, z0, seed), tx);
		const float x01 = lerp(latticeValue(x0, y0, z0 + 1, seed), latticeValue(x0 + 1, y0, z0 + 1, seed), tx);
		const float x11 = lerp(latticeValue(x0, y0 + 1, z0 + 1, seed), latticeValue(x0 + 1, y0 + 1, z0 + 1, seed), tx);
		return lerp(lerp(x00, x10, ty), lerp(x01, x11, ty), tz);
	}

	float fractalNoise(gen_model::gen_types::Point3 point, std::uint32_t seed) {
		float total = 0.0f;
		float amplitude = 0.5f;
		float frequency = 1.0f;
		float amplitudeTotal = 0.0f;
		for (int octave = 0; octave < 4; ++octave) {
			total += valueNoise(point * frequency, seed + static_cast<std::uint32_t>(octave) * 1013u) * amplitude;
			amplitudeTotal += amplitude;
			frequency *= 2.0f;
			amplitude *= 0.5f;
		}
		return total / amplitudeTotal;
	}

	void validate(const gen_model::asteroid::Settings& settings) {
		if (settings.latitudeSegments < 3 || settings.latitudeSegments > 128) {
			throw std::invalid_argument("latitudeSegments must be between 3 and 128");
		}
		if (settings.longitudeSegments < 3 || settings.longitudeSegments > 256) {
			throw std::invalid_argument("longitudeSegments must be between 3 and 256");
		}
                if (settings.minRadius <= 0.0f || settings.maxRadius < settings.minRadius) {
                        throw std::invalid_argument("radius bounds must be positive and ordered");
		}
		if (settings.textureWidth < 2 || settings.textureWidth > 4096 || settings.textureHeight < 2 || settings.textureHeight > 2048) {
			throw std::invalid_argument("texture dimensions must be between 2 and 4096 wide and 2 and 2048 high");
		}
	}

	gen_model::gen_types::Point3 directionAt(float u, float v) {
		if (v <= 0.0f) {
			return {0.0f, 1.0f, 0.0f};
		}
		if (v >= 1.0f) {
			return {0.0f, -1.0f, 0.0f};
		}
		const float longitude = u * 2.0f * PI;
		const float latitude = (0.5f - v) * PI;
		const float cosLatitude = std::cos(latitude);
		return {
			cosLatitude * std::cos(longitude),
			std::sin(latitude),
			cosLatitude * std::sin(longitude)
		};
	}

        struct Crater {
                gen_model::gen_types::Point3 center;
                float cosineRadius;
                float depth;
                float rimHeight;
        };

        float hashUnit(int x, int y, int z, std::uint32_t seed) {
                return static_cast<float>(hashLattice(x, y, z, seed)) / 4294967295.0f;
        }

        std::vector<Crater> generateCraters(std::uint32_t seed, const gen_model::asteroid::CraterBandSettings& settings, int seedOffset) {
                std::vector<Crater> craters;
                craters.reserve(static_cast<std::size_t>(std::max(0, settings.count)));
                for (int index = 0; index < settings.count; ++index) {
                        const int sampleIndex = index + seedOffset;
                        const float y = hashUnit(sampleIndex, 0, 0, seed) * 2.0f - 1.0f;
                        const float longitude = hashUnit(sampleIndex, 1, 0, seed) * 2.0f * PI;
                        const float radial = std::sqrt(std::max(0.0f, 1.0f - y * y));
                        const float radius = settings.minRadius + (settings.maxRadius - settings.minRadius) * hashUnit(sampleIndex, 2, 0, seed);
                        craters.push_back({
                                {radial * std::cos(longitude), y, radial * std::sin(longitude)},
                                std::cos(radius),
                                settings.minDepth + hashUnit(sampleIndex, 3, 0, seed) * (settings.maxDepth - settings.minDepth),
                                settings.minRimHeight + hashUnit(sampleIndex, 4, 0, seed) * (settings.maxRimHeight - settings.minRimHeight)
                        });
                }
                return craters;
        }

        float craterHeight(gen_model::gen_types::Point3 direction, const std::vector<Crater>& craters) {
                float height = 0.0f;
                for (const Crater& crater : craters) {
                        const float cosineDistance = gen_model::gen_types::dot(direction, crater.center);
                        if (cosineDistance <= crater.cosineRadius)
                                continue;
                        const float distance = std::sqrt(std::clamp(
                                (1.0f - cosineDistance) / (1.0f - crater.cosineRadius),
                                0.0f,
                                1.0f
                        ));
                        const float bowlDistance = distance / 0.72f;
                        if (bowlDistance < 1.0f)
                                height -= crater.depth * (1.0f - smoothStep(bowlDistance));
                        const float rimDistance = std::abs(distance - 0.82f) / 0.18f;
                        if (rimDistance < 1.0f)
                                height += crater.rimHeight * (1.0f - smoothStep(rimDistance));
                }
                return height;
        }

        struct SurfaceFeatures {
                std::vector<Crater> macroCraters;
                std::vector<Crater> mediumCraters;
                std::vector<Crater> fineCraters;
        };

        SurfaceFeatures generateSurfaceFeatures(std::uint32_t seed, const gen_model::asteroid::Settings& settings) {
                return {
                        generateCraters(seed, settings.macroCraters, 0),
                        generateCraters(seed + 17u, settings.mediumCraters, 1000),
                        generateCraters(seed + 31u, settings.fineCraters, 2000)
                };
        }

        float surfaceHeight(gen_model::gen_types::Point3 direction, const gen_model::asteroid::Settings& settings, const SurfaceFeatures& features) {
                const float broadScale = settings.useMacroGeometry ? settings.macroFeatureFrequency : 3.0f;
                const float ridgeScale = settings.useMacroGeometry ? settings.mediumFeatureFrequency : 12.0f;
                const float grainScale = settings.useMacroGeometry ? settings.microFeatureFrequency : 48.0f;
                const float broadNoise = fractalNoise(direction * broadScale, settings.seed + 1543u);
                const float ridgedNoise = 1.0f - std::abs(fractalNoise(direction * ridgeScale, settings.seed + 7919u) * 2.0f - 1.0f);
                const float grainNoise = fractalNoise(direction * grainScale, settings.seed + 12347u);
                float height = broadNoise * settings.broadFeatureStrength
                        + (ridgedNoise - 0.5f) * settings.ridgeFeatureStrength
                        + (grainNoise - 0.5f) * settings.grainFeatureStrength
                        + craterHeight(direction, features.fineCraters);
                if (!settings.useMacroGeometry)
                        return height;
                return height
                        + craterHeight(direction, features.macroCraters) * 1.45f
                        + craterHeight(direction, features.mediumCraters) * 1.05f;
        }

	float macroSurfaceHeight(gen_model::gen_types::Point3 direction, const gen_model::asteroid::Settings& settings, const SurfaceFeatures& features) {
		const float broadNoise = fractalNoise(direction * settings.macroFeatureFrequency, settings.seed + 1543u);
		const float ridgeNoise = 1.0f - std::abs(fractalNoise(direction * settings.mediumFeatureFrequency, settings.seed + 7919u) * 2.0f - 1.0f);
		const float ravineNoise = 1.0f - std::abs(fractalNoise(direction * (settings.mediumFeatureFrequency * 2.0f), settings.seed + 23111u) * 2.0f - 1.0f);
		return broadNoise * 0.42f + (ridgeNoise - 0.5f) * 0.32f + (ravineNoise - 0.5f) * 0.14f
			+ craterHeight(direction, features.macroCraters) * 1.55f
			+ craterHeight(direction, features.mediumCraters) * 0.85f;
	}

	gen_model::gen_types::Point3 surfaceColor(gen_model::gen_types::Point3 direction, const gen_model::asteroid::Settings& settings, const SurfaceFeatures& features, float height) {
		if (!settings.useMacroGeometry)
			return {
				48.0f + height * 120.0f,
				43.0f + height * 100.0f,
				38.0f + height * 80.0f
			};

		const float broadColor = fractalNoise(direction * (settings.macroFeatureFrequency * 2.0f), settings.seed + 18431u);
		const float mediumColor = fractalNoise(direction * settings.mediumFeatureFrequency, settings.seed + 19333u);
		const float ravineColor = 1.0f - fractalNoise(direction * (settings.mediumFeatureFrequency * 1.5f), settings.seed + 20707u);
		const float macroRelief = craterHeight(direction, features.macroCraters);
		const float mediumRelief = craterHeight(direction, features.mediumCraters);
		const float macroBasin = std::clamp(-macroRelief * 8.0f, 0.0f, 1.0f);
		const float macroRim = std::clamp(macroRelief * 8.0f, 0.0f, 1.0f);
		const float mediumBasin = std::clamp(-mediumRelief * 12.0f, 0.0f, 1.0f);
		const float stonePatch = std::clamp((mediumColor - 0.35f) * 1.8f, 0.0f, 1.0f);
		const float ravine = std::clamp((ravineColor - 0.45f) * 1.8f, 0.0f, 1.0f);
		const auto blend = [](gen_model::gen_types::Point3 left, gen_model::gen_types::Point3 right, float amount) {
			return left * (1.0f - amount) + right * amount;
		};
		gen_model::gen_types::Point3 color = blend({26.0f, 31.0f, 36.0f}, {112.0f, 68.0f, 34.0f}, 0.18f + broadColor * 0.62f);
		color = blend(color, {108.0f, 101.0f, 83.0f}, mediumColor * 0.42f);
		color = blend(color, {150.0f, 82.0f, 38.0f}, macroRim * 0.65f);
		color = blend(color, {18.0f, 25.0f, 32.0f}, macroBasin * 0.75f);
		color = blend(color, {31.0f, 36.0f, 39.0f}, mediumBasin * 0.55f);
		color = blend(color, {16.0f, 22.0f, 28.0f}, ravine * 0.62f);
		return blend(color, {124.0f, 119.0f, 101.0f}, stonePatch * 0.35f);
	}

	std::uint8_t encodeNormalComponent(float value) {
		return static_cast<std::uint8_t>(std::clamp((value * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
	}

	void appendTriangle(gen_model::gen_types::MeshData& mesh, const std::vector<int>& normalOwners, std::array<int, 3> positions, std::array<int, 3> texcoords) {
		gen_model::gen_types::Point3 normal = gen_model::gen_types::cross(mesh.positions[positions[1]] - mesh.positions[positions[0]], mesh.positions[positions[2]] - mesh.positions[positions[0]]);
		const gen_model::gen_types::Point3 center = (mesh.positions[positions[0]] + mesh.positions[positions[1]] + mesh.positions[positions[2]]) * (1.0f / 3.0f);
		if (gen_model::gen_types::dot(normal, center) < 0.0f) {
			std::swap(positions[1], positions[2]);
			std::swap(texcoords[1], texcoords[2]);
			normal = normal * -1.0f;
		}
		for (const int positionIndex : positions) {
			const int normalOwner = normalOwners[static_cast<std::size_t>(positionIndex)];
			mesh.normals[static_cast<std::size_t>(normalOwner)] = mesh.normals[static_cast<std::size_t>(normalOwner)] + normal;
		}
		mesh.triangles.push_back(gen_model::gen_types::Triangle{
			positions,
			texcoords,
			{
				normalOwners[static_cast<std::size_t>(positions[0])],
				normalOwners[static_cast<std::size_t>(positions[1])],
				normalOwners[static_cast<std::size_t>(positions[2])]
			}
		});
	}
}

gen_model::gen_types::AssetData gen_model::asteroid::generate(const gen_model::asteroid::Settings& settings) {
	validate(settings);
	gen_model::gen_types::AssetData asset;
	const SurfaceFeatures features = generateSurfaceFeatures(settings.seed, settings);
	const int rowWidth = settings.longitudeSegments + 1;
	asset.mesh.positions.reserve(static_cast<std::size_t>((settings.latitudeSegments + 1) * rowWidth));
	asset.mesh.texcoords.reserve(asset.mesh.positions.capacity());
	asset.mesh.normals.reserve(asset.mesh.positions.capacity());
	asset.mesh.triangles.reserve(static_cast<std::size_t>(settings.longitudeSegments * (settings.latitudeSegments * 2 - 2)));

        std::vector<float> radialSamples;
        radialSamples.reserve(asset.mesh.positions.capacity());
        float minimumSample = 1.0f;
        float maximumSample = 0.0f;
        for (int row = 0; row <= settings.latitudeSegments; ++row) {
                const float v = static_cast<float>(row) / static_cast<float>(settings.latitudeSegments);
                for (int column = 0; column <= settings.longitudeSegments; ++column) {
                        const float u = static_cast<float>(column) / static_cast<float>(settings.longitudeSegments);
                        const gen_model::gen_types::Point3 direction = directionAt(u, v);
						const float sample = settings.useMacroGeometry
							? macroSurfaceHeight(direction, settings, features)
							: fractalNoise(direction * 3.0f, settings.seed);
                        radialSamples.push_back(sample);
                        minimumSample = std::min(minimumSample, sample);
                        maximumSample = std::max(maximumSample, sample);
                        asset.mesh.texcoords.push_back({u, v});
                }
        }
        const float sampleRange = maximumSample - minimumSample;
        for (std::size_t index = 0; index < radialSamples.size(); ++index) {
                const float normalizedSample = sampleRange > 0.0f
                        ? (radialSamples[index] - minimumSample) / sampleRange
                        : 0.5f;
                const float radius = settings.minRadius + (settings.maxRadius - settings.minRadius) * normalizedSample;
                const int row = static_cast<int>(index) / rowWidth;
                const int column = static_cast<int>(index) % rowWidth;
                const float u = static_cast<float>(column) / static_cast<float>(settings.longitudeSegments);
                const float v = static_cast<float>(row) / static_cast<float>(settings.latitudeSegments);
                asset.mesh.positions.push_back(directionAt(u, v) * radius);
        }
	std::vector<int> normalOwners(asset.mesh.positions.size());
	asset.mesh.normals.assign(asset.mesh.positions.size(), {0.0f, 0.0f, 0.0f});
	for (int row = 0; row <= settings.latitudeSegments; ++row) {
		for (int column = 0; column <= settings.longitudeSegments; ++column) {
			const int canonicalColumn = column == settings.longitudeSegments ? 0 : column;
			const int canonicalColumnIndex = row * rowWidth + canonicalColumn;
			const int positionIndex = row * rowWidth + column;
			normalOwners[static_cast<std::size_t>(positionIndex)] = canonicalColumnIndex;
		}
	}
	for (int column = 1; column <= settings.longitudeSegments; ++column) {
		normalOwners[static_cast<std::size_t>(column)] = 0;
		const int southPole = settings.latitudeSegments * rowWidth;
		normalOwners[static_cast<std::size_t>(southPole + column)] = southPole;
	}

	for (int row = 0; row < settings.latitudeSegments; ++row) {
		for (int column = 0; column < settings.longitudeSegments; ++column) {
			const int a = row * rowWidth + column;
			const int b = a + 1;
			const int d = (row + 1) * rowWidth + column;
			const int c = d + 1;
			if (row == 0) {
				appendTriangle(asset.mesh, normalOwners, {a, d, c}, {a, d, c});
				continue;
			}
			if (row == settings.latitudeSegments - 1) {
				appendTriangle(asset.mesh, normalOwners, {a, b, d}, {a, b, d});
				continue;
			}
			appendTriangle(asset.mesh, normalOwners, {a, b, c}, {a, b, c});
			appendTriangle(asset.mesh, normalOwners, {a, c, d}, {a, c, d});
		}
	}
	for (std::size_t index = 0; index < asset.mesh.normals.size(); ++index) {
		if (normalOwners[index] == static_cast<int>(index)) {
			asset.mesh.normals[index] = gen_model::gen_types::normalize(asset.mesh.normals[index]);
		}
	}
	for (std::size_t index = 0; index < asset.mesh.normals.size(); ++index) {
		asset.mesh.normals[index] = asset.mesh.normals[static_cast<std::size_t>(normalOwners[index])];
	}

        const std::size_t texelCount = static_cast<std::size_t>(settings.textureWidth * settings.textureHeight);
        std::vector<float> heights(texelCount);
        asset.texture.width = settings.textureWidth;
        asset.texture.height = settings.textureHeight;
        asset.texture.rgba.resize(texelCount * 4);
        for (int row = 0; row < settings.textureHeight; ++row) {
                const float v = static_cast<float>(row) / static_cast<float>(settings.textureHeight - 1);
                for (int column = 0; column < settings.textureWidth; ++column) {
                        const float u = static_cast<float>(column) / static_cast<float>(settings.textureWidth - 1);
                        const gen_model::gen_types::Point3 direction = directionAt(u, v);
                        const float height = surfaceHeight(direction, settings, features);
                        const gen_model::gen_types::Point3 color = surfaceColor(direction, settings, features, height);
                        const std::size_t texel = static_cast<std::size_t>(row * settings.textureWidth + column);
                        const std::size_t offset = texel * 4;
                        heights[texel] = height;
                        asset.texture.rgba[offset] = static_cast<std::uint8_t>(std::clamp(color.x, 0.0f, 255.0f));
                        asset.texture.rgba[offset + 1] = static_cast<std::uint8_t>(std::clamp(color.y, 0.0f, 255.0f));
                        asset.texture.rgba[offset + 2] = static_cast<std::uint8_t>(std::clamp(color.z, 0.0f, 255.0f));
                        asset.texture.rgba[offset + 3] = 255;
                }
        }

        asset.normalMap.width = settings.textureWidth;
        asset.normalMap.height = settings.textureHeight;
        asset.normalMap.rgba.resize(texelCount * 4);
	        const int lastColumn = settings.textureWidth - 1;
        const int lastRow = settings.textureHeight - 1;
        for (int row = 0; row < settings.textureHeight; ++row) {
                const int previousRow = std::max(0, row - 1);
                const int nextRow = std::min(lastRow, row + 1);
                for (int column = 0; column < settings.textureWidth; ++column) {
                        const int previousColumn = column == 0 ? lastColumn - 1 : column - 1;
                        const int nextColumn = column == lastColumn ? 1 : column + 1;
						const float slopeU = (heights[static_cast<std::size_t>(row * settings.textureWidth + nextColumn)] - heights[static_cast<std::size_t>(row * settings.textureWidth + previousColumn)]) * settings.normalMapStrength;
						const float slopeV = (heights[static_cast<std::size_t>(nextRow * settings.textureWidth + column)] - heights[static_cast<std::size_t>(previousRow * settings.textureWidth + column)]) * settings.normalMapStrength;
                        const gen_model::gen_types::Point3 normal = gen_model::gen_types::normalize({-slopeU, -slopeV, 1.0f});
                        const std::size_t offset = static_cast<std::size_t>((row * settings.textureWidth + column) * 4);
                        asset.normalMap.rgba[offset] = encodeNormalComponent(normal.x);
                        asset.normalMap.rgba[offset + 1] = encodeNormalComponent(normal.y);
                        asset.normalMap.rgba[offset + 2] = encodeNormalComponent(normal.z);
                        asset.normalMap.rgba[offset + 3] = 255;
                }
        }
	return asset;
}
