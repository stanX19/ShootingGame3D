#ifndef GEN_MODEL_SPACESHIP_CONFIG_HPP
#define GEN_MODEL_SPACESHIP_CONFIG_HPP

#include <filesystem>
#include <istream>
#include <vector>

#include "spaceship_generator.hpp"

namespace gen_model::spaceship {
	std::vector<Settings> loadCatalog(std::istream& input);
	std::vector<Settings> loadCatalog(const std::filesystem::path& path);
	void writeGenerationReport(const GeneratedShip& ship, const std::filesystem::path& path);
} // namespace gen_model::spaceship

#endif // GEN_MODEL_SPACESHIP_CONFIG_HPP

