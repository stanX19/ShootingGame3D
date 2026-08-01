#ifndef GEN_MODEL_ASTEROID_GENERATOR_HPP
#define GEN_MODEL_ASTEROID_GENERATOR_HPP

#include <cstdint>

#include "gen_types.hpp"

namespace gen_model::asteroid {

	struct Settings {
		std::uint32_t seed = 1337;
		int latitudeSegments = 24;
		int longitudeSegments = 36;
		float minRadius = 0.95f;
		float maxRadius = 1.05f;
		int textureWidth = 2048;
		int textureHeight = 1024;
	};

	gen_model::gen_types::AssetData generate(const Settings& settings = {});

} // namespace gen_model::asteroid

#endif // GEN_MODEL_ASTEROID_GENERATOR_HPP
