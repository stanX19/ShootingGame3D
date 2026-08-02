#include "asteroid_big.hpp"

gen_model::asteroid::Settings gen_model::asteroid::big::settings() {
	gen_model::asteroid::Settings result;
	result.seed = 7331;
	result.latitudeSegments = 16;
	result.longitudeSegments = 24;
	result.minRadius = 0.8f;
	result.maxRadius = 1.0f;
	result.textureWidth = 2048;
	result.textureHeight = 1024;
	result.useMacroGeometry = true;
	result.macroCraters = {10, 0.14f, 0.25f, 0.035f, 0.070f, 0.025f, 0.050f};
	result.mediumCraters = {36, 0.055f, 0.12f, 0.020f, 0.045f, 0.014f, 0.030f};
	result.fineCraters = {80, 0.020f, 0.065f, 0.010f, 0.025f, 0.006f, 0.015f};
	result.broadFeatureStrength = 0.40f;
	result.ridgeFeatureStrength = 0.26f;
	result.grainFeatureStrength = 0.08f;
	result.normalMapStrength = 64.0f;
	result.macroFeatureFrequency = 0.48f;
	result.mediumFeatureFrequency = 4.8f;
	result.microFeatureFrequency = 48.0f;
	return result;
}

gen_model::gen_types::AssetData gen_model::asteroid::big::generate() {
	return gen_model::asteroid::generate(settings());
}
