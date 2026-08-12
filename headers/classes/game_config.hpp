#ifndef GAME_CONFIG_HPP
#define GAME_CONFIG_HPP

#include <initializer_list>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config/spaceship_config.hpp"
#include "json.hpp"
#include "raylib.h"

class GameConfig {
public:
	using RootSource = std::pair<std::string, std::string>;

	GameConfig() = default;
	~GameConfig() = default;

	void init(const std::vector<RootSource>& sources);
	void init(std::initializer_list<RootSource> sources);
	void initConstants();
	bool isLoaded() const { return loaded; }

	virtual float getFloat(const std::string& path, float defaultVal) const;
	virtual int getInt(const std::string& path, int defaultVal) const;
	virtual bool getBool(const std::string& path, bool defaultVal) const;
	virtual std::string getString(
		const std::string& path,
		const std::string& defaultVal
	) const;
	virtual Vector3 getVector3(const std::string& path, Vector3 defaultVal) const;

	void setFloat(const std::string& path, float value);
	void setBool(const std::string& path, bool value);
	void setString(const std::string& path, const std::string& value);

	void saveRoot(const std::string& rootName);
	void saveChanged();
	void saveAll();

	const nlohmann::json& getJson() const { return config; }
	nlohmann::json getSection(const std::string& path) const;
	class SubGameConfig getSubConfig(const std::string& path) const;

	const config::SpaceshipConfig& spaceship() const noexcept {
		return spaceshipConfig;
	}

	float ARENA_SIZE = 2000.0f;
	int COMBAT_DIST = 1000;
	int UNIT_COUNT = 4;

	struct Physics {
		float collisionElasticity = 0.5f;
		float maxAngularKick = 0.5f;
		float roughness = 2.5f;
	} physics;

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
	struct RootJsonFile {
		std::string sourcePath;
		bool dirty = false;
	};

	nlohmann::json config;
	std::map<std::string, RootJsonFile> roots;
	config::SpaceshipConfig spaceshipConfig;
	bool loaded = false;

	const nlohmann::json* navigatePath(const std::string& path) const;
	nlohmann::json* navigatePath(
		nlohmann::json& root,
		const std::string& path
	) const;
	void setJsonValue(const std::string& path, nlohmann::json value);
	void saveRootJsonFile(const std::string& rootName, RootJsonFile& file);
};

class SubGameConfig : public GameConfig {
public:
	SubGameConfig(const GameConfig* parent, const std::string& root)
		: parentCfg(parent), rootPath(root) {}

	float getFloat(const std::string& path, float defaultVal) const override {
		return parentCfg->getFloat(rootPath + "." + path, defaultVal);
	}
	int getInt(const std::string& path, int defaultVal) const override {
		return parentCfg->getInt(rootPath + "." + path, defaultVal);
	}
	bool getBool(const std::string& path, bool defaultVal) const override {
		return parentCfg->getBool(rootPath + "." + path, defaultVal);
	}
	std::string getString(
		const std::string& path,
		const std::string& defaultVal
	) const override {
		return parentCfg->getString(rootPath + "." + path, defaultVal);
	}
	Vector3 getVector3(const std::string& path, Vector3 defaultVal) const override {
		return parentCfg->getVector3(rootPath + "." + path, defaultVal);
	}
	nlohmann::json getSection(const std::string& path) const {
		return parentCfg->getSection(rootPath + "." + path);
	}
	SubGameConfig getSubConfig(const std::string& path) const {
		return parentCfg->getSubConfig(rootPath + "." + path);
	}

private:
	void setFloat(const std::string&, float) = delete;
	void setBool(const std::string&, bool) = delete;
	void setString(const std::string&, const std::string&) = delete;
	void saveRoot(const std::string&) = delete;
	void saveChanged() = delete;
	void saveAll() = delete;
	void init(const std::vector<RootSource>&) = delete;
	void init(std::initializer_list<RootSource>) = delete;
	void initConstants() = delete;

	const GameConfig* parentCfg;
	std::string rootPath;
};

inline SubGameConfig GameConfig::getSubConfig(const std::string& path) const {
	return SubGameConfig(this, path);
}

#endif // GAME_CONFIG_HPP
