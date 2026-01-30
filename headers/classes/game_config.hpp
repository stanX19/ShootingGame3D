#ifndef GAME_CONFIG_HPP
#define GAME_CONFIG_HPP

#include <string>
#include <unordered_map>
#include "json.hpp"
#include "raylib.h"

class GameConfig {
public:
	GameConfig() = default;
	~GameConfig() = default;

	void init(const std::string& configPath);
	bool isLoaded() const { return loaded; }

	// Generic getters with defaults
	float getFloat(const std::string& path, float defaultVal) const;
	int getInt(const std::string& path, int defaultVal) const;
	bool getBool(const std::string& path, bool defaultVal) const;
	std::string getString(const std::string& path, const std::string& defaultVal) const;
	Vector3 getVector3(const std::string& path, Vector3 defaultVal) const;

	// Access raw JSON for arrays/complex structures
	const nlohmann::json& getJson() const { return config; }
	nlohmann::json getSection(const std::string& path) const;

private:
	nlohmann::json config;
	bool loaded = false;

	// Helper to navigate nested JSON by dot-separated path (e.g., "weapons.bullet.machineGun.damage")
	const nlohmann::json* navigatePath(const std::string& path) const;
};

#endif  // GAME_CONFIG_HPP
