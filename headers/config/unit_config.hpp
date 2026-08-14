#ifndef CONFIG_UNIT_CONFIG_HPP
#define CONFIG_UNIT_CONFIG_HPP

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "json.hpp"

namespace config {

class SpaceshipConfig;

class UnitConfig {
public:
	struct Stats {
		float collisionRadius = 1.0f;
		float hp = 0.0f;
		float hpRegen = 0.0f;
		float shield = 0.0f;
		float shieldRegen = 0.0f;
		float damage = 0.0f;
		float maxSpeed = 0.0f;
		float turnSpeed = 0.0f;
		float mass = 1.0f;
		int score = 0;
		int killedScore = 0;
	};

	struct Effects {
		float explosionRadiusScale = 1.0f;
		float deathSoundRadiusScale = 1.0f;
	};

	struct Definition {
		std::string id;
		std::string spaceshipReference;
		Stats stats;
		Effects effects;
		bool elite = false;
	};

	void init(
		const nlohmann::json& root,
		std::string_view sourcePath,
		const SpaceshipConfig& spaceships
	);

	bool contains(std::string_view id) const;
	const Definition& get(std::string_view id) const;
	std::vector<std::string> ids() const;

private:
	static Stats parseStats(const nlohmann::json& value, std::string_view path);
	static Effects parseEffects(const nlohmann::json& value, std::string_view path);
	static Definition parseDefinition(
		std::string_view id,
		const nlohmann::json& value,
		std::string_view sourcePath,
		const SpaceshipConfig& spaceships
	);
	static void validateDefinition(
		const Definition& definition,
		std::string_view sourcePath
	);

	std::map<std::string, Definition> definitions;
};

} // namespace config

#endif // CONFIG_UNIT_CONFIG_HPP
