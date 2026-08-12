#include "config/spaceship_config.hpp"

#include <cmath>
#include <stdexcept>

#include "raymath.h"

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

	float number(const nlohmann::json& object, const char* key, std::string_view path) {
		const auto& value = required(object, key, path);
		if (!value.is_number())
			throw std::invalid_argument(std::string(path) + ": " + key + " must be numeric");
		const float result = value.get<float>();
		if (!std::isfinite(result))
			throw std::invalid_argument(std::string(path) + ": " + key + " must be finite");
		return result;
	}

	float optionalNumber(
		const nlohmann::json& value,
		const nlohmann::json& defaults,
		const char* key,
		std::string_view path
	) {
		if (value.contains(key))
			return number(value, key, path);
		return number(defaults, key, "defaults");
	}
}

namespace config {

Vector3 SpaceshipConfig::parseVector(
	const nlohmann::json& value,
	std::string_view path
) {
	if (!value.is_object())
		throw std::invalid_argument(std::string(path) + ": vector must be an object");
	return Vector3{
		number(value, "x", path),
		number(value, "y", path),
		number(value, "z", path)
	};
}

Vector3 SpaceshipConfig::parseForward(
	const nlohmann::json& value,
	std::string_view path
) {
	const Vector3 forward = parseVector(value, path);
	if (Vector3LengthSqr(forward) <= 0.000001f)
		throw std::invalid_argument(std::string(path) + ": forward vector cannot be zero");
	return Vector3Normalize(forward);
}

SpaceshipConfig::Engine SpaceshipConfig::parseEngine(
	const nlohmann::json& value,
	std::string_view path
) {
	Engine engine;
	engine.center = parseVector(
		required(value, "center", path),
		std::string(path) + ".center"
	);
	engine.radius = number(value, "radius", path);
	engine.length = number(value, "length", path);
	engine.nozzleDepth = number(value, "nozzleDepth", path);
	if (engine.radius <= 0.0f || engine.length <= 0.0f || engine.nozzleDepth <= 0.0f)
		throw std::invalid_argument(std::string(path) + ": invalid engine dimensions");
	return engine;
}

SpaceshipConfig::Mount SpaceshipConfig::parseMount(
	const nlohmann::json& value,
	const nlohmann::json& defaults,
	std::string_view path
) {
	const auto& id = required(value, "id", path);
	if (!id.is_string())
		throw std::invalid_argument(std::string(path) + ": id must be a string");

	Mount mount;
	mount.diagnosticId = id.get<std::string>();
	mount.position = parseVector(
		required(value, "position", path),
		std::string(path) + ".position"
	);
	mount.forward = parseForward(
		required(value, "forward", path),
		std::string(path) + ".forward"
	);
	mount.supportRoot = parseVector(
		required(value, "supportRoot", path),
		std::string(path) + ".supportRoot"
	);
	mount.turretRadius = optionalNumber(value, defaults, "turretRadius", path);
	mount.barrelRadius = optionalNumber(value, defaults, "barrelRadius", path);
	mount.barrelLength = optionalNumber(value, defaults, "barrelLength", path);
	mount.traverseHalfAngleDegrees = optionalNumber(
		value, defaults, "traverseHalfAngleDegrees", path
	);
	mount.supportWidth = number(value, "supportWidth", path);
	mount.supportHeight = number(value, "supportHeight", path);
	mount.socketHeight = number(value, "socketHeight", path);
	if (mount.turretRadius <= 0.0f || mount.barrelRadius <= 0.0f
		|| mount.barrelLength <= 0.0f || mount.traverseHalfAngleDegrees <= 0.0f
		|| mount.supportWidth <= 0.0f || mount.supportHeight <= 0.0f
		|| mount.socketHeight <= 0.0f)
		throw std::invalid_argument(std::string(path) + ": invalid mount dimensions");
	return mount;
}

SpaceshipConfig::Definition SpaceshipConfig::parseDefinition(
	std::string_view id,
	const nlohmann::json& value,
	const nlohmann::json& defaults,
	std::string_view sourcePath
) {
	const std::string path = "ships." + std::string(id);
	const auto& modelPath = required(value, "modelPath", path);
	if (!modelPath.is_string() || modelPath.get<std::string>().empty())
		throw std::invalid_argument(path + ": modelPath must be a non-empty string");
	const auto& runtime = required(value, "runtime", path);
	const auto& engines = required(value, "engines", path);
	const auto& mounts = required(value, "mounts", path);
	if (!engines.is_array() || engines.empty() || !mounts.is_array() || mounts.empty())
		throw std::invalid_argument(std::string(sourcePath) + ": " + path + ": empty geometry");

	Definition definition;
	definition.id = id;
	definition.modelPath = modelPath.get<std::string>();
	definition.modelRadius = number(runtime, "modelRadius", path + ".runtime");
	for (std::size_t index = 0; index < engines.size(); ++index)
		definition.engines.push_back(parseEngine(
			engines.at(index),
			path + ".engines[" + std::to_string(index) + "]"
		));
	for (std::size_t index = 0; index < mounts.size(); ++index)
		definition.mounts.push_back(parseMount(
			mounts.at(index),
			defaults,
			path + ".mounts[" + std::to_string(index) + "]"
		));
	validateDefinition(definition, sourcePath);
	return definition;
}

void SpaceshipConfig::validateDefinition(
	const Definition& definition,
	std::string_view sourcePath
) {
	if (definition.id.empty() || definition.modelPath.empty()
		|| !std::isfinite(definition.modelRadius) || definition.modelRadius <= 0.0f)
		throw std::invalid_argument(
			std::string(sourcePath) + ": invalid ship definition " + definition.id
		);
}

void SpaceshipConfig::init(
	const nlohmann::json& root,
	std::string_view sourcePath
) {
	if (!root.is_object() || root.value("schemaVersion", 0) != 1)
		throw std::invalid_argument(std::string(sourcePath) + ": schemaVersion must be 1");
	const auto& defaults = required(root, "defaults", sourcePath);
	const auto& ships = required(root, "ships", sourcePath);
	if (!defaults.is_object() || !ships.is_object() || ships.empty())
		throw std::invalid_argument(
			std::string(sourcePath) + ": defaults/ships must be non-empty objects"
		);

	std::map<std::string, Definition> parsed;
	for (auto iterator = ships.begin(); iterator != ships.end(); ++iterator)
		parsed.emplace(
			iterator.key(),
			parseDefinition(iterator.key(), iterator.value(), defaults, sourcePath)
		);
	definitions = std::move(parsed);
}

const SpaceshipConfig::Definition& SpaceshipConfig::get(std::string_view id) const {
	const auto iterator = definitions.find(std::string(id));
	if (iterator == definitions.end())
		throw std::out_of_range("SPACESHIP: unknown ship ID: " + std::string(id));
	return iterator->second;
}

} // namespace config
