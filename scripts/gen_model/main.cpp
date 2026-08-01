#include "asset_writer.hpp"
#include "asteroid_generator.hpp"
#include "asteroid_big.hpp"
#include "asteroid_small.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
	enum class Profile {
		Small,
		Big
	};

	struct Options {
		std::filesystem::path outputDirectory = "assets/Models/asteroid";
		Profile profile = Profile::Small;
		bool singleProfile = false;
		std::string basename;
		int latitudeSegments = 0;
		int longitudeSegments = 0;
		int textureWidth = 0;
		int textureHeight = 0;
	};

	int parseInteger(const char* option, const char* value) {
		try {
			return std::stoi(value);
		} catch (const std::exception&) {
			throw std::invalid_argument(std::string("Invalid value for ") + option);
		}
	}

	Options parseOptions(int argc, char** argv) {
		Options options;
		if (argc == 1) {
			return options;
		}
		if (argc == 2 && argv[1][0] != '-') {
			options.outputDirectory = argv[1];
			return options;
		}
		for (int index = 1; index < argc; ++index) {
			const std::string_view argument = argv[index];
			if (argument == "--output-dir" || argument == "--basename" || argument == "--profile" || argument == "--latitude" || argument == "--longitude" || argument == "--texture-width" || argument == "--texture-height") {
				if (index + 1 >= argc) {
					throw std::invalid_argument("Missing value for " + std::string(argument));
				}
				const char* value = argv[++index];
				if (argument == "--output-dir") {
					options.outputDirectory = value;
					continue;
				}
				if (argument == "--basename") {
					options.basename = value;
					continue;
				}
				if (argument == "--profile") {
					if (std::string_view(value) == "small") {
						options.profile = Profile::Small;
					} else if (std::string_view(value) == "big") {
						options.profile = Profile::Big;
					} else {
						throw std::invalid_argument("Profile must be small or big");
					}
					options.singleProfile = true;
					continue;
				}
				int& target = argument == "--latitude" ? options.latitudeSegments
					: argument == "--longitude" ? options.longitudeSegments
					: argument == "--texture-width" ? options.textureWidth
					: options.textureHeight;
				target = parseInteger(argv[index - 1], value);
				continue;
			}
			throw std::invalid_argument("Unknown option: " + std::string(argument));
		}
		if (!options.singleProfile) {
			throw std::invalid_argument("--profile is required when using generator options");
		}
		return options;
	}

	gen_model::asteroid::Settings applyOverrides(Options options) {
		auto settings = options.profile == Profile::Big
			? gen_model::asteroid::big::settings()
			: gen_model::asteroid::small::settings();
		if (options.latitudeSegments > 0) settings.latitudeSegments = options.latitudeSegments;
		if (options.longitudeSegments > 0) settings.longitudeSegments = options.longitudeSegments;
		if (options.textureWidth > 0) settings.textureWidth = options.textureWidth;
		if (options.textureHeight > 0) settings.textureHeight = options.textureHeight;
		return settings;
	}
}

int main(int argc, char** argv) {
	try {
		const Options options = parseOptions(argc, argv);
		const std::filesystem::path outputDirectory = options.outputDirectory;
		if (options.singleProfile) {
			const auto settings = applyOverrides(options);
			const std::string basename = options.basename.empty()
				? options.profile == Profile::Big ? "asteroid_big" : "asteroid_small"
				: options.basename;
			gen_model::writeAsteroidAssets(gen_model::asteroid::generate(settings), outputDirectory, basename);
			std::cout << "Generated " << basename << " in " << outputDirectory.string() << '\n';
			return 0;
		}
		const auto smallAsset = gen_model::asteroid::small::generate();
		gen_model::writeAsteroidAssets(smallAsset, outputDirectory, "generated_asteroid");
		gen_model::writeAsteroidAssets(smallAsset, outputDirectory, "asteroid_small");
		gen_model::writeAsteroidAssets(gen_model::asteroid::big::generate(), outputDirectory, "asteroid_big");
		std::cout << "Generated generated_asteroid, asteroid_small, and asteroid_big assets in " << outputDirectory.string() << '\n';
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "gen_model: " << error.what() << '\n';
		return 1;
	}
}
