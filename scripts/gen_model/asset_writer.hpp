#ifndef GEN_MODEL_ASSET_WRITER_HPP
#define GEN_MODEL_ASSET_WRITER_HPP

#include <filesystem>

#include "gen_types.hpp"

namespace gen_model {
	void writeAsteroidAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory);
} // namespace gen_model

#endif // GEN_MODEL_ASSET_WRITER_HPP
