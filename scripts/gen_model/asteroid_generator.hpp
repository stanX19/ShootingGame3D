#ifndef GEN_MODEL_ASTEROID_GENERATOR_HPP
#define GEN_MODEL_ASTEROID_GENERATOR_HPP

#include <cstdint>

#include "gen_types.hpp"

namespace gen_model::asteroid {

	struct CraterBandSettings {
		int count = 64;
		float minRadius = 0.025f;
		float maxRadius = 0.09f;
		float minDepth = 0.018f;
		float maxDepth = 0.053f;
		float minRimHeight = 0.012f;
		float maxRimHeight = 0.030f;
	};

	struct Settings {
		std::uint32_t seed = 1337;
		int latitudeSegments = 24;
		int longitudeSegments = 36;
		float minRadius = 0.8f;
		float maxRadius = 1.0f;
		int textureWidth = 2048;
		int textureHeight = 1024;
		bool useMacroGeometry = false;
		CraterBandSettings macroCraters{0, 0.14f, 0.25f, 0.035f, 0.070f, 0.025f, 0.050f};
		CraterBandSettings mediumCraters{0, 0.055f, 0.12f, 0.020f, 0.045f, 0.014f, 0.030f};
		CraterBandSettings fineCraters{};
		float broadFeatureStrength = 0.24f;
		float ridgeFeatureStrength = 0.38f;
		float grainFeatureStrength = 0.12f;
		float normalMapStrength = 12.0f;
		float macroFeatureFrequency = 2.5f;
		float mediumFeatureFrequency = 12.0f;
		float microFeatureFrequency = 48.0f;
	};

	gen_model::gen_types::AssetData generate(const Settings& settings = {});

} // namespace gen_model::asteroid

#endif // GEN_MODEL_ASTEROID_GENERATOR_HPP
