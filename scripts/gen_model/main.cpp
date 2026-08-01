#include "asset_writer.hpp"
#include "asteroid_generator.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
	try {
		if (argc > 2) {
			throw std::invalid_argument("Usage: gen_model [output-directory]");
		}
		const std::filesystem::path outputDirectory = argc == 2
			? std::filesystem::path(argv[1])
			: std::filesystem::path("assets/Models/asteroid");
		gen_model::writeAsteroidAssets(gen_model::asteroid::generate(), outputDirectory);
		std::cout << "Generated asteroid assets in " << outputDirectory.string() << '\n';
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "gen_model: " << error.what() << '\n';
		return 1;
	}
}
