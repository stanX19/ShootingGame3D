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
}

TEST_CASE("Spaceship factory scales the unit-radius model by collision radius", "[integration][spaceship]") {
	GameConfig config;
	config.init(kConfigRoots);
	const auto& authored = config.spaceship().get("player");

	CHECK(spaceship::factory::detail::visualScaleForAuthoredModel(
		1.0f,
		authored.modelRadius
	) == Catch::Approx(1.0f));
	CHECK(spaceship::factory::detail::visualScaleForAuthoredModel(
		0.5f,
		authored.modelRadius
	) == Catch::Approx(0.5f));
	CHECK(spaceship::factory::detail::visualScaleForAuthoredModel(
		1.5f,
		authored.modelRadius
	) == Catch::Approx(1.5f));
	CHECK(spaceship::factory::detail::visualScaleForAuthoredModel(
		100.123f,
		authored.modelRadius
	) == Catch::Approx(100.123f));
}

TEST_CASE("GameConfig rejects an unknown spaceship ID", "[integration][spaceship]") {
	GameConfig config;
	config.init(kConfigRoots);
	REQUIRE_THROWS_AS(config.spaceship().get("does_not_exist"), std::out_of_range);
}

TEST_CASE("GameConfig preserves dotted reads and scoped subconfigs", "[integration][config]") {
	GameConfig config;
	config.init(kConfigRoots);

	CHECK(config.getFloat("weapons.missile.weapons.nuke.instantRadius", 0.0f)
		== Catch::Approx(5.0f));
	const SubGameConfig nuke =
		config.getSubConfig("weapons.missile.weapons.nuke");
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
