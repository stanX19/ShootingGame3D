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
	void initConstants();
	bool isLoaded() const { return loaded; }

	// Generic getters with defaults
	virtual float getFloat(const std::string& path, float defaultVal) const;
	virtual int getInt(const std::string& path, int defaultVal) const;
	virtual bool getBool(const std::string& path, bool defaultVal) const;
	virtual std::string getString(const std::string& path, const std::string& defaultVal) const;
	virtual Vector3 getVector3(const std::string& path, Vector3 defaultVal) const;
	
	void setFloat(const std::string& path, float value);
	void setBool(const std::string& path, bool value);
	void setString(const std::string& path, const std::string& value);
	
	void save(const std::string& configPath);

	const nlohmann::json& getJson() const { return config; }
	nlohmann::json getSection(const std::string& path) const;
	
	class SubGameConfig getSubConfig(const std::string& path) const;

	// Game constants
	float ARENA_SIZE = 2000.0f;
	int COMBAT_DIST = 1000;
	int UNIT_COUNT = 4;

	struct Physics {
		float collisionElasticity = 0.5f;
		float maxAngularKick = 0.5f;
		float roughness = 2.5f;
	} physics;

	// Settings
	struct Settings {
		bool showHPBar = true;
		float masterVolume = 0.5f;
		float controlSensitivity = 1.0f;
	} settings;

	struct PlayerLoadout {
		std::string w1;
		std::string w2;
		std::string w3;
		std::string w4;
		std::string special;
	} loadout;

	struct Debug {
		bool showTarget = false;
	} debug;

private:
	nlohmann::json config;
	bool loaded = false;

	// Helper to navigate nested JSON by dot-separated path (e.g., "weapons.bullet.weapons.machineGun.damage")
	const nlohmann::json* navigatePath(const std::string& path) const;
};

class SubGameConfig : public GameConfig {
public:
	SubGameConfig(const GameConfig* parent, const std::string& root) : parentCfg(parent), rootPath(root) {}
	
	float getFloat(const std::string& path, float defaultVal) const override {
		return parentCfg->getFloat(rootPath + "." + path, defaultVal);
	}
	int getInt(const std::string& path, int defaultVal) const override {
		return parentCfg->getInt(rootPath + "." + path, defaultVal);
	}
	bool getBool(const std::string& path, bool defaultVal) const override {
		return parentCfg->getBool(rootPath + "." + path, defaultVal);
	}
	std::string getString(const std::string& path, const std::string& defaultVal) const override {
		return parentCfg->getString(rootPath + "." + path, defaultVal);
	}
	Vector3 getVector3(const std::string& path, Vector3 defaultVal) const override {
		return parentCfg->getVector3(rootPath + "." + path, defaultVal);
	}

private:
	// Explicitly hide modification methods
	void setFloat(const std::string& path, float value) = delete;
	void setBool(const std::string& path, bool value) = delete;
	void save(const std::string& configPath) = delete;
	void init(const std::string& configPath) = delete;
	void initConstants() = delete;

	const GameConfig* parentCfg;
	std::string rootPath;
};

inline SubGameConfig GameConfig::getSubConfig(const std::string& path) const {
	return SubGameConfig(this, path);
}

#endif  // GAME_CONFIG_HPP
