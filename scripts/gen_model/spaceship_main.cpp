#include "asset_writer.hpp"
#include "spaceship_config.hpp"

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

namespace {
	constexpr std::string_view CATALOG_PATH = "assets/config/spaceships.json";
	constexpr std::string_view OUTPUT_ROOT = "../scratch/model-qc/spaceships";

	std::string fingerprintLabel(std::uint64_t fingerprint) {
		std::ostringstream label;
		label << std::hex << std::setfill('0') << std::setw(16) << fingerprint;
		return label.str();
	}
}

int main() {
	try {
		const auto catalog = gen_model::spaceship::loadCatalog(CATALOG_PATH);
		for (const auto& settings : catalog) {
			const auto generated = gen_model::spaceship::generate(settings);
			const std::filesystem::path outputDirectory = std::filesystem::path(OUTPUT_ROOT)
				/ (settings.id + "_" + fingerprintLabel(generated.settingsFingerprint));
			const std::string basename = "spaceship_" + settings.id;
			gen_model::writeSpaceshipAssets(generated.asset, outputDirectory, basename);
			gen_model::spaceship::writeGenerationReport(generated, outputDirectory / "generation_report.json");
			std::cout << "Generated " << settings.id << " -> " << outputDirectory.string() << '\n';
		}
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "gen_spaceships: " << error.what() << '\n';
		return 1;
	}
}
