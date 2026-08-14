#include "game_config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "raylib.h"

void GameConfig::init(std::initializer_list<RootSource> sources) {
	init(std::vector<RootSource>(sources));
}

void GameConfig::init(const std::vector<RootSource>& sources) {
	if (loaded)
		return;
	if (sources.empty())
		throw std::invalid_argument("CONFIG: at least one root source is required");

	nlohmann::json candidate = nlohmann::json::object();
	std::map<std::string, RootJsonFile> candidateFiles;

	for (const auto& [rootName, sourcePath] : sources) {
		if (rootName.empty() || sourcePath.empty())
			throw std::invalid_argument("CONFIG: root name and source path are required");
		if (!candidateFiles.emplace(rootName, RootJsonFile{sourcePath, false}).second)
			throw std::invalid_argument("CONFIG: duplicate root: " + rootName);

		std::ifstream file(sourcePath);
		if (!file)
			throw std::runtime_error(
				"CONFIG: failed to open root " + rootName + ": " + sourcePath
			);
		try {
			file >> candidate[rootName];
		} catch (const nlohmann::json::parse_error& error) {
			throw std::runtime_error(
				"CONFIG: failed to parse root " + rootName + ": " + error.what()
			);
		}
	}

	config::SpaceshipConfig candidateSpaceship;
	const auto spaceshipRoot = candidate.find("spaceship");
	if (spaceshipRoot != candidate.end())
		candidateSpaceship.init(
			*spaceshipRoot,
			candidateFiles.at("spaceship").sourcePath
		);

	config::UnitConfig candidateUnits;
	const auto unitsRoot = candidate.find("units");
	if (unitsRoot != candidate.end()) {
		if (spaceshipRoot == candidate.end())
			throw std::invalid_argument(
				"CONFIG: units root requires a spaceship root"
			);
		candidateUnits.init(
			*unitsRoot,
			candidateFiles.at("units").sourcePath,
			candidateSpaceship
		);
	}

	config = std::move(candidate);
	roots = std::move(candidateFiles);
	spaceshipConfig = std::move(candidateSpaceship);
	unitConfig = std::move(candidateUnits);
	loaded = true;
	initConstants();
}

void GameConfig::initConstants() {
	ARENA_SIZE = getFloat("game.arenaSize", 2000.0f);
	COMBAT_DIST = getInt("game.combatDist", 1000);
	UNIT_COUNT = getInt("game.unitCount", 4);

	physics.collisionElasticity = getFloat("physics.collisionElasticity", 0.5f);
	physics.maxAngularKick = getFloat("physics.maxAngularKick", 0.5f);
	physics.roughness = getFloat("physics.roughness", 2.5f);

	settings.showHPBar = getBool("settings.showHPBar", true);
	settings.masterVolume = getFloat("audio.masterVolume", 0.5f);
	settings.controlSensitivity = Clamp(
		getFloat("settings.controlSensitivity", 1.0f), 0.01f, 1.0f
	);
	loadout.w1 = getString("loadout.w1", "bullet.machineGun");
	loadout.w2 = getString("loadout.w2", "bullet.machineGun");
	loadout.w3 = getString("loadout.w3", "lazer.basic");
	loadout.w4 = getString("loadout.w4", "lazer.basic");
	loadout.special = getString("loadout.special", "missile.basic");

	debug.showTarget = getBool("debug.showTarget", false);
}

const nlohmann::json* GameConfig::navigatePath(const std::string& path) const {
	if (!loaded)
		return nullptr;

	const nlohmann::json* current = &config;
	std::istringstream stream(path);
	std::string token;
	while (std::getline(stream, token, '.')) {
		if (token.empty() || !current->is_object() || !current->contains(token))
			return nullptr;
		current = &(*current)[token];
	}
	return current;
}

nlohmann::json* GameConfig::navigatePath(
	nlohmann::json& root,
	const std::string& path
) const {
	if (path.empty())
		return nullptr;

	nlohmann::json* current = &root;
	std::istringstream stream(path);
	std::string token;
	while (std::getline(stream, token, '.')) {
		if (token.empty())
			return nullptr;
		if (!current->is_object())
			*current = nlohmann::json::object();
		current = &(*current)[token];
	}
	return current;
}

float GameConfig::getFloat(const std::string& path, float defaultVal) const {
	const nlohmann::json* node = navigatePath(path);
	if (node == nullptr || !node->is_number())
		return defaultVal;
	return node->get<float>();
}

int GameConfig::getInt(const std::string& path, int defaultVal) const {
	const nlohmann::json* node = navigatePath(path);
	if (node == nullptr || !node->is_number())
		return defaultVal;
	return node->get<int>();
}

bool GameConfig::getBool(const std::string& path, bool defaultVal) const {
	const nlohmann::json* node = navigatePath(path);
	if (node == nullptr || !node->is_boolean())
		return defaultVal;
	return node->get<bool>();
}

std::string GameConfig::getString(
	const std::string& path,
	const std::string& defaultVal
) const {
	const nlohmann::json* node = navigatePath(path);
	if (node == nullptr || !node->is_string())
		return defaultVal;
	return node->get<std::string>();
}

Vector3 GameConfig::getVector3(
	const std::string& path,
	Vector3 defaultVal
) const {
	const nlohmann::json* node = navigatePath(path);
	if (node == nullptr || !node->is_object())
		return defaultVal;
	return Vector3{
		node->value("x", defaultVal.x),
		node->value("y", defaultVal.y),
		node->value("z", defaultVal.z)
	};
}

nlohmann::json GameConfig::getSection(const std::string& path) const {
	const nlohmann::json* node = navigatePath(path);
	return node == nullptr ? nlohmann::json{} : *node;
}

void GameConfig::setJsonValue(const std::string& path, nlohmann::json value) {
	const std::size_t separator = path.find('.');
	if (separator == std::string::npos || separator == 0 || separator + 1 >= path.size())
		throw std::invalid_argument(
			"CONFIG: setters require a root-qualified path: " + path
		);
	const std::string rootName = path.substr(0, separator);
	auto root = roots.find(rootName);
	if (root == roots.end())
		throw std::invalid_argument("CONFIG: unknown root: " + rootName);

	nlohmann::json updatedRoot = config.at(rootName);
	nlohmann::json* node = navigatePath(
		updatedRoot,
		path.substr(separator + 1)
	);
	if (node == nullptr)
		throw std::invalid_argument("CONFIG: invalid path: " + path);
	if (*node == value)
		return;
	*node = std::move(value);

	config::SpaceshipConfig updatedSpaceship = spaceshipConfig;
	config::UnitConfig updatedUnits = unitConfig;
	if (rootName == "spaceship") {
		updatedSpaceship.init(updatedRoot, root->second.sourcePath);
		const auto unitsRoot = config.find("units");
		if (unitsRoot != config.end()) {
			const auto unitsFile = roots.find("units");
			if (unitsFile == roots.end())
				throw std::logic_error("CONFIG: units root has no source file");
			updatedUnits.init(
				*unitsRoot,
				unitsFile->second.sourcePath,
				updatedSpaceship
			);
		}
	}
	if (rootName == "units") {
		const auto spaceshipRoot = config.find("spaceship");
		if (spaceshipRoot == config.end())
			throw std::invalid_argument(
				"CONFIG: units root requires a spaceship root"
			);
		updatedUnits.init(
			updatedRoot,
			root->second.sourcePath,
			spaceshipConfig
		);
	}
	config[rootName] = std::move(updatedRoot);
	spaceshipConfig = std::move(updatedSpaceship);
	unitConfig = std::move(updatedUnits);
	root->second.dirty = true;
	initConstants();
}

void GameConfig::setFloat(const std::string& path, float value) {
	setJsonValue(path, value);
}

void GameConfig::setString(const std::string& path, const std::string& value) {
	setJsonValue(path, value);
}

void GameConfig::setBool(const std::string& path, bool value) {
	setJsonValue(path, value);
}

void GameConfig::saveRootJsonFile(
	const std::string& rootName,
	RootJsonFile& file
) {
	std::ofstream output(file.sourcePath, std::ios::trunc);
	if (!output)
		throw std::runtime_error(
			"CONFIG: failed to open root for saving " + rootName + ": " + file.sourcePath
		);
	output << config.at(rootName).dump(4) << '\n';
	if (!output)
		throw std::runtime_error(
			"CONFIG: failed while saving root " + rootName + ": " + file.sourcePath
		);
	file.dirty = false;
	TraceLog(
		LOG_INFO,
		"CONFIG: Saved root %s to %s",
		rootName.c_str(),
		file.sourcePath.c_str()
	);
}

void GameConfig::saveRoot(const std::string& rootName) {
	auto iterator = roots.find(rootName);
	if (iterator == roots.end())
		throw std::invalid_argument("CONFIG: unknown root: " + rootName);
	saveRootJsonFile(rootName, iterator->second);
}

void GameConfig::saveChanged() {
	for (auto& [rootName, file] : roots) {
		if (file.dirty)
			saveRootJsonFile(rootName, file);
	}
}

void GameConfig::saveAll() {
	for (auto& [rootName, file] : roots)
		saveRootJsonFile(rootName, file);
}
