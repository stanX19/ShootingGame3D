#include "game_config.hpp"
#include <fstream>
#include <sstream>
#include "raylib.h"

void GameConfig::init(const std::string& configPath) {
	if (loaded) return;

	std::ifstream file(configPath);
	if (!file.is_open()) {
		TraceLog(LOG_WARNING, "CONFIG: Failed to open config file: %s", configPath.c_str());
		return;
	}

	try {
		file >> config;
		loaded = true;
		TraceLog(LOG_INFO, "CONFIG: Loaded config from %s", configPath.c_str());
	} catch (const nlohmann::json::parse_error& e) {
		TraceLog(LOG_WARNING, "CONFIG: Failed to parse config: %s", e.what());
		return;
	}

	initConstants();
}

void GameConfig::initConstants() {
	ARENA_SIZE = getFloat("game.arenaSize", 2000.0f);
	COMBAT_DIST = getFloat("game.combatDist", 1000.0f);
	UNIT_COUNT = getInt("game.unitCount", 4);
	
	physics.collisionElasticity = getFloat("physics.collisionElasticity", 0.5f);
	physics.maxAngularKick = getFloat("physics.maxAngularKick", 0.5f);
	physics.roughness = getFloat("physics.roughness", 2.5f);

	settings.showHPBar = getBool("settings.showHPBar", true);
	settings.masterVolume = getFloat("audio.masterVolume", 0.5f);
	settings.controlSensitivity = Clamp(getFloat("settings.controlSensitivity", 1.0f), 0.01f, 1.0f);
	loadout.w1 = getString("loadout.w1", "bullet.machineGun");
	loadout.w2 = getString("loadout.w2", "bullet.machineGun");
	loadout.w3 = getString("loadout.w3", "lazer.basic");
	loadout.w4 = getString("loadout.w4", "lazer.basic");
	loadout.special = getString("loadout.special", "missile.basic");

	debug.showTarget = getBool("debug.showTarget", false);
}

const nlohmann::json* GameConfig::navigatePath(const std::string& path) const {
	if (!loaded) return nullptr;

	const nlohmann::json* current = &config;
	std::istringstream ss(path);
	std::string token;

	while (std::getline(ss, token, '.')) {
		if (!current->is_object() || !current->contains(token)) {
			return nullptr;
		}
		current = &(*current)[token];
	}
	return current;
}

float GameConfig::getFloat(const std::string& path, float defaultVal) const {
	const nlohmann::json* node = navigatePath(path);
	if (!node || !node->is_number()) return defaultVal;
	return node->get<float>();
}

int GameConfig::getInt(const std::string& path, int defaultVal) const {
	const nlohmann::json* node = navigatePath(path);
	if (!node || !node->is_number()) return defaultVal;
	return node->get<int>();
}

bool GameConfig::getBool(const std::string& path, bool defaultVal) const {
	const nlohmann::json* node = navigatePath(path);
	if (!node || !node->is_boolean()) return defaultVal;
	return node->get<bool>();
}

std::string GameConfig::getString(const std::string& path, const std::string& defaultVal) const {
	const nlohmann::json* node = navigatePath(path);
	if (!node || !node->is_string()) return defaultVal;
	return node->get<std::string>();
}

Vector3 GameConfig::getVector3(const std::string& path, Vector3 defaultVal) const {
	const nlohmann::json* node = navigatePath(path);
	if (!node || !node->is_object()) return defaultVal;

	return Vector3{
		node->value("x", defaultVal.x),
		node->value("y", defaultVal.y),
		node->value("z", defaultVal.z)
	};
}

nlohmann::json GameConfig::getSection(const std::string& path) const {
	const nlohmann::json* node = navigatePath(path);
	if (!node) return nlohmann::json{};
	return *node;
}

void GameConfig::setFloat(const std::string& path, float value) {
	std::istringstream ss(path);
	std::string token;
	nlohmann::json* current = &config;

	while (std::getline(ss, token, '.')) {
		current = &((*current)[token]);
	}
	*current = value;
	initConstants(); // Re-sync cached constants
}

void GameConfig::setString(const std::string& path, const std::string& value) {
	std::istringstream ss(path);
	std::string token;
	nlohmann::json* current = &config;

	while (std::getline(ss, token, '.')) {
		current = &((*current)[token]);
	}
	*current = value;
	initConstants(); // Re-sync cached constants
}

void GameConfig::setBool(const std::string& path, bool value) {
	std::istringstream ss(path);
	std::string token;
	nlohmann::json* current = &config;

	while (std::getline(ss, token, '.')) {
		current = &((*current)[token]);
	}
	*current = value;
	initConstants(); // Re-sync cached constants
}

void GameConfig::save(const std::string& configPath) {
	std::ofstream file(configPath);
	if (!file.is_open()) {
		TraceLog(LOG_WARNING, "CONFIG: Failed to open config file for saving: %s", configPath.c_str());
		return;
	}
	file << config.dump(4);
	TraceLog(LOG_INFO, "CONFIG: Saved config to %s", configPath.c_str());
}
