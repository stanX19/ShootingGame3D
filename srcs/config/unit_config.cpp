#include "config/unit_config.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "config/spaceship_config.hpp"

namespace {

const nlohmann::json& required(
	const nlohmann::json& object,
	const char* key,
	std::string_view path
) {
	if (!object.is_object() || !object.contains(key))
		throw std::invalid_argument(std::string(path) + ": missing " + key);
	return object.at(key);
}

float number(
	const nlohmann::json& object,
	const char* key,
	std::string_view path
) {
	const nlohmann::json& value = required(object, key, path);
	if (!value.is_number())
		throw std::invalid_argument(
			std::string(path) + ": " + key + " must be numeric"
		);
	const float result = value.get<float>();
	if (!std::isfinite(result))
		throw std::invalid_argument(
			std::string(path) + ": " + key + " must be finite"
		);
	return result;
}

int integer(
	const nlohmann::json& object,
	const char* key,
	std::string_view path
) {
	const nlohmann::json& value = required(object, key, path);
	if (!value.is_number_integer())
		throw std::invalid_argument(
			std::string(path) + ": " + key + " must be an integer"
		);
	return value.get<int>();
}

bool boolean(
	const nlohmann::json& object,
	const char* key,
	std::string_view path
) {
	const nlohmann::json& value = required(object, key, path);
	if (!value.is_boolean())
		throw std::invalid_argument(
			std::string(path) + ": " + key + " must be boolean"
		);
	return value.get<bool>();
}

std::string stringValue(
	const nlohmann::json& object,
	const char* key,
	std::string_view path
) {
	const nlohmann::json& value = required(object, key, path);
	if (!value.is_string() || value.get<std::string>().empty())
		throw std::invalid_argument(
			std::string(path) + ": " + key + " must be a non-empty string"
		);
	return value.get<std::string>();
}

} // namespace

namespace config {

UnitConfig::Stats UnitConfig::parseStats(
	const nlohmann::json& value,
	std::string_view path
) {
	if (!value.is_object())
		throw std::invalid_argument(std::string(path) + ": stats must be an object");

	Stats stats;
	stats.collisionRadius = number(value, "collisionRadius", path);
	stats.hp = number(value, "hp", path);
	stats.hpRegen = number(value, "hpRegen", path);
	stats.shield = number(value, "shield", path);
	stats.shieldRegen = number(value, "shieldRegen", path);
	stats.damage = number(value, "damage", path);
	stats.maxSpeed = number(value, "maxSpeed", path);
	stats.turnSpeed = number(value, "turnSpeed", path);
	stats.mass = number(value, "mass", path);
	stats.score = integer(value, "score", path);
	stats.killedScore = integer(value, "killedScore", path);
	return stats;
}

UnitConfig::Effects UnitConfig::parseEffects(
	const nlohmann::json& value,
	std::string_view path
) {
	if (!value.is_object())
		throw std::invalid_argument(std::string(path) + ": effects must be an object");

	Effects effects;
	effects.explosionRadiusScale = number(value, "explosionRadiusScale", path);
	effects.deathSoundRadiusScale = number(value, "deathSoundRadiusScale", path);
	return effects;
}

UnitConfig::Definition UnitConfig::parseDefinition(
	std::string_view id,
	const nlohmann::json& value,
	std::string_view sourcePath,
	const SpaceshipConfig& spaceships
) {
	const std::string path = "definitions." + std::string(id);
	if (!value.is_object())
		throw std::invalid_argument(
			std::string(sourcePath) + ": " + path + " must be an object"
		);

	Definition definition;
	definition.id = id;
	definition.spaceshipReference =
		stringValue(value, "spaceshipReference", path);
	definition.stats = parseStats(
		required(value, "stats", path),
		path + ".stats"
	);
	definition.effects = parseEffects(
		required(value, "effects", path),
		path + ".effects"
	);
	definition.elite = boolean(value, "elite", path);

	try {
		spaceships.get(definition.spaceshipReference);
	} catch (const std::out_of_range&) {
		throw std::invalid_argument(
			std::string(sourcePath) + ": " + path
			+ ".spaceshipReference points to an unknown spaceship: "
			+ definition.spaceshipReference
		);
	}

	validateDefinition(definition, sourcePath);
	return definition;
}

void UnitConfig::validateDefinition(
	const Definition& definition,
	std::string_view sourcePath
) {
	const Stats& stats = definition.stats;
	const Effects& effects = definition.effects;
	if (stats.collisionRadius <= 0.0f
		|| stats.hp <= 0.0f
		|| stats.hpRegen < 0.0f
		|| stats.shield < 0.0f
		|| stats.shieldRegen < 0.0f
		|| stats.damage < 0.0f
		|| stats.maxSpeed <= 0.0f
		|| stats.turnSpeed < 0.0f
		|| stats.mass <= 0.0f
		|| stats.score < 0
		|| stats.killedScore < 0
		|| effects.explosionRadiusScale <= 0.0f
		|| effects.deathSoundRadiusScale < 0.0f)
		throw std::invalid_argument(
			std::string(sourcePath) + ": invalid numeric values for unit "
			+ definition.id
		);
}

void UnitConfig::init(
	const nlohmann::json& root,
	std::string_view sourcePath,
	const SpaceshipConfig& spaceships
) {
	if (!root.is_object() || root.value("schemaVersion", 0) != 2)
		throw std::invalid_argument(
			std::string(sourcePath) + ": schemaVersion must be 2"
		);
	const nlohmann::json& values = required(root, "definitions", sourcePath);
	if (!values.is_object() || values.empty())
		throw std::invalid_argument(
			std::string(sourcePath) + ": definitions must be a non-empty object"
		);

	std::map<std::string, Definition> parsed;
	for (auto iterator = values.begin(); iterator != values.end(); ++iterator)
		parsed.emplace(
			iterator.key(),
			parseDefinition(iterator.key(), iterator.value(), sourcePath, spaceships)
		);
	definitions = std::move(parsed);
}

bool UnitConfig::contains(std::string_view id) const {
	return definitions.find(std::string(id)) != definitions.end();
}

const UnitConfig::Definition& UnitConfig::get(std::string_view id) const {
	const auto iterator = definitions.find(std::string(id));
	if (iterator == definitions.end())
		throw std::out_of_range("UNIT: unknown unit ID: " + std::string(id));
	return iterator->second;
}

std::vector<std::string> UnitConfig::ids() const {
	std::vector<std::string> result;
	result.reserve(definitions.size());
	for (const auto& entry : definitions)
		result.push_back(entry.first);
	return result;
}

} // namespace config
