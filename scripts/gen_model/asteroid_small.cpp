#include "asteroid_small.hpp"

gen_model::asteroid::Settings gen_model::asteroid::small::settings() {
	gen_model::asteroid::Settings result;
	result.seed = 1337;
	result.latitudeSegments = 12;
	result.longitudeSegments = 18;
	result.minRadius = 0.8f;
	result.maxRadius = 1.0f;
	result.textureWidth = 2048;
	result.textureHeight = 1024;
	result.useMacroGeometry = false;
	result.macroCraters = {0, 0.14f, 0.25f, 0.035f, 0.070f, 0.025f, 0.050f};
	result.mediumCraters = {0, 0.055f, 0.12f, 0.020f, 0.045f, 0.014f, 0.030f};
	result.fineCraters = {64, 0.025f, 0.09f, 0.018f, 0.053f, 0.012f, 0.030f};
	result.broadFeatureStrength = 0.24f;
	result.ridgeFeatureStrength = 0.38f;
	result.grainFeatureStrength = 0.12f;
	result.normalMapStrength = 12.0f;
	result.macroFeatureFrequency = 2.5f;
	result.mediumFeatureFrequency = 12.0f;
	result.microFeatureFrequency = 48.0f;
	return result;
}

gen_model::gen_types::AssetData gen_model::asteroid::small::generate() {
	return gen_model::asteroid::generate(settings());
}
