#include "game_config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "raylib.h"

void GameConfig::init(std::initializer_list<RootSource> sources) {
	init(std::vector<RootSource>(sources));
}

void GameConfig::init(const std::vector<RootSource>& sources) {
	if (m_loaded)
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

	m_config = std::move(candidate);
	m_roots = std::move(candidateFiles);
	m_spaceshipConfig = std::move(candidateSpaceship);
	m_unitConfig = std::move(candidateUnits);
	m_loaded = true;
	initConstants();
}

void GameConfig::initConstants() {
	ARENA_SIZE = getFloat("game.arenaSize", 2000.0f);
	COMBAT_DIST = getInt("game.combatDist", 1000);
	UNIT_COUNT = getInt("game.unitCount", 4);
	deathBodyLifespan = getFloat("game.deathBodyLifespan", 0.35f);

	killAttr.assistThresholdPct = getFloat("game.killAttr.assistThresholdPct", 0.30f);
	killAttr.assistWindowSeconds = getFloat("game.killAttr.assistWindowSeconds", 10.0f);
	killAttr.fallbackAttributionWindowSeconds = getFloat("game.killAttr.fallbackAttributionWindowSeconds", 10.0f);

	hud.damageNumbers.fontSize = getFloat("hud.damageNumbers.fontSize", 20.0f);
	hud.damageNumbers.opacity = getFloat("hud.damageNumbers.opacity", 0.75f);
	hud.damageNumbers.resetCooldown = getFloat("hud.damageNumbers.resetCooldown", 0.45f);

	physics.collisionElasticity = getFloat("physics.collisionElasticity", 0.5f);
	physics.maxAngularKick = getFloat("physics.maxAngularKick", 0.5f);
	physics.roughness = getFloat("physics.roughness", 2.5f);

	settings.showHPBar = getBool("settings.showHPBar", true);
	settings.showDamageNumbers = getBool("settings.showDamageNumbers", true);
	settings.showToasts = getBool("settings.showToasts", true);
	settings.showKillLogs = getBool("settings.showKillLogs", true);
	settings.screenShakeMagnitude = getFloat("settings.screenShakeMagnitude", 1.0f);
	settings.masterVolume = getFloat("audio.masterVolume", 0.5f);
	settings.controlSensitivity = Clamp(
		getFloat("settings.controlSensitivity", 1.0f), 0.01f, 1.0f
	);
	loadout.turretWeapons = getStringArray("loadout.turretWeapons", {});
	loadout.specialWeapon = getString(
		"loadout.specialWeapon",
		"missile.basic"
	);

	debug.showTarget = getBool("debug.showTarget", false);
}

const nlohmann::json* GameConfig::navigatePath(const std::string& path) const {
	if (!m_loaded)
		return nullptr;

	const nlohmann::json* current = &m_config;
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

std::vector<std::string> GameConfig::getStringArray(
	const std::string& path,
	const std::vector<std::string>& defaultVal
) const {
	const nlohmann::json* node = navigatePath(path);
	if (node == nullptr || !node->is_array())
		return defaultVal;
	std::vector<std::string> result;
	result.reserve(node->size());
	for (const auto& value : *node) {
		if (!value.is_string())
			return defaultVal;
		result.push_back(value.get<std::string>());
	}
	return result;
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
	auto root = m_roots.find(rootName);
	if (root == m_roots.end())
		throw std::invalid_argument("CONFIG: unknown root: " + rootName);

	nlohmann::json updatedRoot = m_config.at(rootName);
	nlohmann::json* node = navigatePath(
		updatedRoot,
		path.substr(separator + 1)
	);
	if (node == nullptr)
		throw std::invalid_argument("CONFIG: invalid path: " + path);
	if (*node == value)
		return;
	*node = std::move(value);

	config::SpaceshipConfig updatedSpaceship = m_spaceshipConfig;
	config::UnitConfig updatedUnits = m_unitConfig;
	if (rootName == "spaceship") {
		updatedSpaceship.init(updatedRoot, root->second.sourcePath);
		const auto unitsRoot = m_config.find("units");
		if (unitsRoot != m_config.end()) {
			const auto unitsFile = m_roots.find("units");
			if (unitsFile == m_roots.end())
				throw std::logic_error("CONFIG: units root has no source file");
			updatedUnits.init(
				*unitsRoot,
				unitsFile->second.sourcePath,
				updatedSpaceship
			);
		}
	}
	if (rootName == "units") {
		const auto spaceshipRoot = m_config.find("spaceship");
		if (spaceshipRoot == m_config.end())
			throw std::invalid_argument(
				"CONFIG: units root requires a spaceship root"
			);
		updatedUnits.init(
			updatedRoot,
			root->second.sourcePath,
			m_spaceshipConfig
		);
	}
	m_config[rootName] = std::move(updatedRoot);
	m_spaceshipConfig = std::move(updatedSpaceship);
	m_unitConfig = std::move(updatedUnits);
	root->second.dirty = true;
	initConstants();
}

void GameConfig::setFloat(const std::string& path, float value) {
	setJsonValue(path, value);
}

void GameConfig::setString(const std::string& path, const std::string& value) {
	setJsonValue(path, value);
	if (path == "loadout.specialWeapon")
		loadout.specialWeapon = value;
}

void GameConfig::setStringArray(
	const std::string& path,
	const std::vector<std::string>& value
) {
	nlohmann::json array = nlohmann::json::array();
	for (const std::string& entry : value)
		array.push_back(entry);
	setJsonValue(path, std::move(array));
	if (path == "loadout.turretWeapons")
		loadout.turretWeapons = value;
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
	output << m_config.at(rootName).dump(4) << '\n';
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
	auto iterator = m_roots.find(rootName);
	if (iterator == m_roots.end())
		throw std::invalid_argument("CONFIG: unknown root: " + rootName);
	saveRootJsonFile(rootName, iterator->second);
}

void GameConfig::saveChanged() {
	for (auto& [rootName, file] : m_roots) {
		if (file.dirty)
			saveRootJsonFile(rootName, file);
	}
}

void GameConfig::saveAll() {
	for (auto& [rootName, file] : m_roots)
		saveRootJsonFile(rootName, file);
}
