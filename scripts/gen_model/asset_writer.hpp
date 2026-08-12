#ifndef GEN_MODEL_ASSET_WRITER_HPP
#define GEN_MODEL_ASSET_WRITER_HPP

#include <filesystem>
#include <string>

#include "gen_types.hpp"

namespace gen_model {
	void writeModelAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory, const std::string& basename);
	void writeSpaceshipAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory, const std::string& basename);
	void writeAsteroidAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory);
	void writeAsteroidAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory, const std::string& basename);
} // namespace gen_model

#endif // GEN_MODEL_ASSET_WRITER_HPP
