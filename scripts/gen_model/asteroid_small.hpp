#ifndef GEN_MODEL_ASTEROID_SMALL_HPP
#define GEN_MODEL_ASTEROID_SMALL_HPP

#include "asteroid_generator.hpp"

namespace gen_model::asteroid::small {
	gen_model::asteroid::Settings settings();
	gen_model::gen_types::AssetData generate();
}

#endif // GEN_MODEL_ASTEROID_SMALL_HPP
