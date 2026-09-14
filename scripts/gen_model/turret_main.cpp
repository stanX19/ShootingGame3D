#include "asset_writer.hpp"
#include "turret_generator.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
	constexpr std::string_view DEFAULT_OUTPUT_ROOT = "../scratch/model-qc/turrets";

	std::string_view archetypeString(gen_model::turret::Archetype archetype) {
		switch (archetype) {
			case gen_model::turret::Archetype::BasicShooter: return "basic_shooter";
			case gen_model::turret::Archetype::HeavyMgRifle: return "heavy_mg_rifle";
			case gen_model::turret::Archetype::LazerDeletor: return "lazer_deletor";
			case gen_model::turret::Archetype::LazerShotgun: return "lazer_shotgun";
		}
		return "basic_shooter";
	}

	void writeReportJson(const gen_model::turret::GenerationReport& report, const std::filesystem::path& path) {
		std::ofstream out(path);
		if (!out) {
			throw std::runtime_error("Unable to write generation report: " + path.string());
		}
		out << "{\n"
			<< "  \"id\": \"" << report.id << "\",\n"
			<< "  \"archetype\": \"" << archetypeString(report.archetype) << "\",\n"
			<< "  \"vertexCount\": " << report.vertexCount << ",\n"
			<< "  \"triangleCount\": " << report.triangleCount << ",\n"
			<< "  \"boundingRadius\": " << report.boundingRadius << ",\n"
			<< "  \"boundingHeight\": " << report.boundingHeight << ",\n"
			<< "  \"muzzlePosition\": {\"x\": " << report.muzzlePosition.x << ", \"y\": " << report.muzzlePosition.y << ", \"z\": " << report.muzzlePosition.z << "},\n"
			<< "  \"pivotPosition\": {\"x\": " << report.pivotPosition.x << ", \"y\": " << report.pivotPosition.y << ", \"z\": " << report.pivotPosition.z << "}\n"
			<< "}\n";
	}
}

int main(int argc, char* argv[]) {
	try {
		std::filesystem::path outputRoot = DEFAULT_OUTPUT_ROOT;

		for (int i = 1; i < argc; ++i) {
			const std::string_view arg = argv[i];
			if (arg == "--output-dir" && i + 1 < argc) {
				outputRoot = argv[++i];
			} else if (arg[0] != '-') {
				outputRoot = arg;
			}
		}

		std::vector<gen_model::turret::Settings> catalog = {
			gen_model::turret::defaultBasicShooter(),
			gen_model::turret::defaultHeavyMgRifle(),
			gen_model::turret::defaultLazerDeletor(),
			gen_model::turret::defaultLazerShotgun()
		};

		for (const auto& settings : catalog) {
			const auto generated = gen_model::turret::generate(settings);
			const std::filesystem::path outputDir = outputRoot / settings.id;
			std::filesystem::create_directories(outputDir);

			const std::string basename = "turret_" + settings.id;
			gen_model::writeTurretAssets(generated.asset, outputDir, basename);
			writeReportJson(generated.report, outputDir / "generation_report.json");

			std::cout << "Generated turret [" << settings.id << "] -> " << outputDir.string()
					  << " (" << generated.report.triangleCount << " tris, radius "
					  << generated.report.boundingRadius << ")\n";
		}
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "gen_turrets error: " << error.what() << '\n';
		return 1;
	}
}
