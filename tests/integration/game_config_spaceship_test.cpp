#include "catch2/catch_amalgamated.hpp"
#include "game_config.hpp"
#include "config/spaceship_config.hpp"
#include "entities/spaceship_factory.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace {

const std::vector<GameConfig::RootSource> kConfigRoots{
	{"audio", "assets/config/audio.json"},
	{"debug", "assets/config/debug.json"},
	{"game", "assets/config/game.json"},
	{"hud", "assets/config/hud.json"},
	{"loadout", "assets/config/loadout.json"},
	{"physics", "assets/config/physics.json"},
	{"settings", "assets/config/settings.json"},
	{"sounds", "assets/config/sounds.json"},
	{"units", "assets/config/units.json"},
	{"weapons", "assets/config/weapons.json"},
	{"spaceship", "assets/config/spaceships.json"}
};

}

TEST_CASE("GameConfig loads spaceship roots into typed definitions", "[integration][spaceship]") {
	GameConfig config;
	config.init(kConfigRoots);

	const auto& player = config.spaceship().get("player");
	CHECK(player.mounts.size() == 4);
	CHECK(player.engines.size() == 2);
	CHECK(player.modelRadius == Catch::Approx(3.8799111843f));

	const auto& terminator = config.spaceship().get("terminator");
	CHECK(terminator.mounts.size() == 32);
	CHECK(terminator.engines.size() == 6);
	CHECK(terminator.modelRadius == Catch::Approx(8.6744432449f));

	const auto& interceptor = config.spaceship().get("interceptor_quad");
	CHECK(interceptor.mounts.size() == 4);
	CHECK(interceptor.engines.size() == 2);

	const auto& heavyQuad = config.spaceship().get("heavy_quad");
	CHECK(heavyQuad.mounts.size() == 4);
	CHECK(heavyQuad.engines.size() == 3);
}

TEST_CASE("GameConfig loads materialized unit definitions", "[integration][unit]") {
	GameConfig config;
	config.init(kConfigRoots);

	const auto& units = config.units();
	CHECK(units.contains("fighter"));
	CHECK(units.contains("basic"));
	CHECK(units.contains("terminator"));
	CHECK(units.contains("heavy_quad"));
	CHECK(units.contains("interceptor_quad"));
	CHECK(units.ids() == std::vector<std::string>{
		"basic",
		"elite",
		"fastElite",
		"fighter",
		"heavy_quad",
		"interceptor_quad",
		"mothership",
		"terminator"
	});
	CHECK(units.get("fighter").spaceshipReference == "player");
	CHECK(units.get("fighter").stats.collisionRadius == Catch::Approx(1.0f));
	CHECK(units.get("fighter").stats.shieldRegen == Catch::Approx(25.0f));
	CHECK(units.get("elite").stats.maxSpeed == Catch::Approx(85.0f));
	CHECK(units.get("heavy_quad").stats.collisionRadius == Catch::Approx(2.4f));
	CHECK(units.get("interceptor_quad").stats.collisionRadius == Catch::Approx(1.6f));
}

TEST_CASE("GameConfig rejects a unit with an unknown spaceship reference", "[integration][unit]") {
	const std::filesystem::path directory =
		std::filesystem::temp_directory_path() / "shooting_game_unit_reference_test";
	std::filesystem::remove_all(directory);
	std::filesystem::create_directories(directory);
	const std::filesystem::path unitsPath = directory / "units.json";

	std::ifstream input("assets/config/units.json");
	REQUIRE(input.good());
	nlohmann::json unitsRoot;
	input >> unitsRoot;
	unitsRoot["definitions"]["player"]["spaceshipReference"] = "missing";
	std::ofstream(unitsPath) << unitsRoot.dump(2);

	GameConfig config;
	REQUIRE_THROWS_AS(
		config.init({
			{"units", unitsPath.string()},
			{"spaceship", "assets/config/spaceships.json"}
		}),
		std::invalid_argument
	);
	std::filesystem::remove_all(directory);
}

TEST_CASE("Spaceship factory scales the unit-radius model by collision radius", "[integration][spaceship]") {
	if (!IsWindowReady()) {
		SetConfigFlags(FLAG_WINDOW_HIDDEN);
		InitWindow(64, 64, "integration test");
	}
	GameConfig config;
	config.init(kConfigRoots);
	ModelManager modelManager;
	const auto mounts1 = spaceship::factory::getModelAndMounts(config, modelManager, "player", 1.0f);
	CHECK(mounts1.bodyScale == Catch::Approx(1.0f));
	const auto mounts05 = spaceship::factory::getModelAndMounts(config, modelManager, "player", 0.5f);
	CHECK(mounts05.bodyScale == Catch::Approx(0.5f));
	const auto mounts15 = spaceship::factory::getModelAndMounts(config, modelManager, "player", 1.5f);
	CHECK(mounts15.bodyScale == Catch::Approx(1.5f));
}

TEST_CASE("GameConfig rejects an unknown spaceship ID", "[integration][spaceship]") {
	GameConfig config;
	config.init(kConfigRoots);
	REQUIRE_THROWS_AS(config.spaceship().get("does_not_exist"), std::out_of_range);
}

TEST_CASE("GameConfig preserves dotted reads and scoped subconfigs", "[integration][config]") {
	GameConfig config;
	config.init(kConfigRoots);

	CHECK(config.getFloat("weapons.weapons.nuke.instantRadius", 0.0f)
		== Catch::Approx(5.0f));
	const SubGameConfig nuke =
		config.getSubConfig("weapons.weapons.nuke");
	CHECK(nuke.getFloat("instantRadius", 0.0f) == Catch::Approx(5.0f));
	CHECK(config.getSection("loadout").is_object());
}

TEST_CASE("GameConfig saves only the root changed by a setter", "[integration][config]") {
	const std::filesystem::path directory =
		std::filesystem::temp_directory_path() / "shooting_game_config_root_test";
	std::filesystem::remove_all(directory);
	std::filesystem::create_directories(directory);
	const std::filesystem::path gamePath = directory / "game.json";
	const std::filesystem::path settingsPath = directory / "settings.json";
	{
		std::ofstream(gamePath) << R"({"arenaSize":100,"combatDist":50,"unitCount":2})";
		std::ofstream(settingsPath) << R"({"showHPBar":true})";
	}

	GameConfig config;
	config.init({
		{"game", gamePath.string()},
		{"settings", settingsPath.string()}
	});
	config.setFloat("game.arenaSize", 125.0f);
	config.saveChanged();

	nlohmann::json savedGame;
	nlohmann::json savedSettings;
	std::ifstream(gamePath) >> savedGame;
	std::ifstream(settingsPath) >> savedSettings;
	CHECK(savedGame.at("arenaSize") == 125.0f);
	CHECK(savedSettings == nlohmann::json{{"showHPBar", true}});
	std::filesystem::remove_all(directory);
}

TEST_CASE("SpaceshipConfig rejects invalid runtime geometry", "[integration][spaceship]") {
	std::ifstream file("assets/config/spaceships.json");
	REQUIRE(file.good());
	nlohmann::json root;
	file >> root;
	root["ships"]["player"]["runtime"]["modelRadius"] = 0.0;

	config::SpaceshipConfig spaceshipConfig;
	REQUIRE_THROWS(spaceshipConfig.init(root, "spaceships.json"));
}
