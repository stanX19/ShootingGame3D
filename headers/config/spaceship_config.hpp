#ifndef CONFIG_SPACESHIP_CONFIG_HPP
#define CONFIG_SPACESHIP_CONFIG_HPP

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "json.hpp"
#include "raylib.h"

namespace config {

class SpaceshipConfig {
public:
	struct Engine {
		Vector3 center{};
		float radius = 0.0f;
		float length = 0.0f;
		float nozzleDepth = 0.0f;
	};

	struct Mount {
		std::string diagnosticId;
		Vector3 position{};
		Vector3 forward{0.0f, 0.0f, 1.0f};
		Vector3 supportRoot{};
		float turretRadius = 0.25f;
		float barrelRadius = 0.25f;
		float barrelLength = 3.25f;
		float traverseHalfAngleDegrees = 45.0f;
		float supportWidth = 0.0f;
		float supportHeight = 0.0f;
		float socketHeight = 0.0f;
	};

	struct Definition {
		std::string id;
		std::string modelPath;
		float modelRadius = 0.0f;
		std::vector<Engine> engines;
		std::vector<Mount> mounts;
	};

	void init(const nlohmann::json& root, std::string_view sourcePath);
	const Definition& get(std::string_view id) const;

private:
	static Vector3 parseVector(const nlohmann::json& value, std::string_view path);
	static Vector3 parseForward(const nlohmann::json& value, std::string_view path);
	static Engine parseEngine(const nlohmann::json& value, std::string_view path);
	static Mount parseMount(
		const nlohmann::json& value,
		const nlohmann::json& defaults,
		std::string_view path
	);
	static Definition parseDefinition(
		std::string_view id,
		const nlohmann::json& value,
		const nlohmann::json& defaults,
		std::string_view sourcePath
	);
	static void validateDefinition(const Definition& definition, std::string_view sourcePath);

	std::map<std::string, Definition> definitions;
};

} // namespace config

#endif // CONFIG_SPACESHIP_CONFIG_HPP
