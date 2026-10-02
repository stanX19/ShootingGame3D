#ifndef GAME_CONFIG_HPP
#define GAME_CONFIG_HPP

#include <initializer_list>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config/spaceship_config.hpp"
#include "config/unit_config.hpp"
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
	bool isLoaded() const { return m_loaded; }

	virtual float getFloat(const std::string& path, float defaultVal) const;
	virtual int getInt(const std::string& path, int defaultVal) const;
	virtual bool getBool(const std::string& path, bool defaultVal) const;
	virtual std::string getString(
		const std::string& path,
		const std::string& defaultVal
	) const;
	virtual std::vector<std::string> getStringArray(
		const std::string& path,
		const std::vector<std::string>& defaultVal
	) const;
	virtual Vector3 getVector3(const std::string& path, Vector3 defaultVal) const;

	void setFloat(const std::string& path, float value);
	void setBool(const std::string& path, bool value);
	void setString(const std::string& path, const std::string& value);
	void setStringArray(
		const std::string& path,
		const std::vector<std::string>& value
	);

	void saveRoot(const std::string& rootName);
	void saveChanged();
	void saveAll();

	const nlohmann::json& getJson() const { return m_config; }
	nlohmann::json getSection(const std::string& path) const;
	class SubGameConfig getSubConfig(const std::string& path) const;

	const config::SpaceshipConfig& spaceship() const noexcept {
		return m_spaceshipConfig;
	}

	const config::UnitConfig& units() const noexcept {
		return m_unitConfig;
	}

	float ARENA_SIZE = 2000.0f;
	int COMBAT_DIST = 1000;
	int UNIT_COUNT = 4;
	float deathBodyLifespan = 0.35f;

	struct KillAttr {
		float assistThresholdPct = 0.30f;
		float assistWindowSeconds = 10.0f;
		float fallbackAttributionWindowSeconds = 10.0f;
	} killAttr;

	struct HUD {
		struct DamageNumbersConfig {
			float fontSize = 20.0f;
			float opacity = 0.75f;
			float resetCooldown = 0.45f;
		} damageNumbers;

		struct WarningToastsConfig {
			int fontSize = 24;
			int boxPaddingX = 20;
			int boxHeight = 34;
			float fillOpacity = 0.3f;
			float slotOffsetY = 10.0f;
		} warningToasts;
	} hud;

	struct Physics {
		float collisionElasticity = 0.5f;
		float maxAngularKick = 0.5f;
		float roughness = 2.5f;
	} physics;

	struct Settings {
		bool showHPBar = true;
		bool showDamageNumbers = true;
		bool showToasts = true;
		bool showKillLogs = true;
		float screenShakeMagnitude = 1.0f;
		float masterVolume = 0.5f;
		float controlSensitivity = 1.0f;
	} settings;

	struct PlayerLoadout {
		std::vector<std::string> turretWeapons;
		std::string specialWeapon;
	} loadout;

	struct Debug {
		bool showTarget = false;
	} debug;

private:
	struct RootJsonFile {
		std::string sourcePath;
		bool dirty = false;
	};

	nlohmann::json m_config;
	std::map<std::string, RootJsonFile> m_roots;
	config::SpaceshipConfig m_spaceshipConfig;
	config::UnitConfig m_unitConfig;
	bool m_loaded = false;

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
		: m_parentCfg(parent), m_rootPath(root) {}

	float getFloat(const std::string& path, float defaultVal) const override {
		return m_parentCfg->getFloat(m_rootPath + "." + path, defaultVal);
	}
	int getInt(const std::string& path, int defaultVal) const override {
		return m_parentCfg->getInt(m_rootPath + "." + path, defaultVal);
	}
	bool getBool(const std::string& path, bool defaultVal) const override {
		return m_parentCfg->getBool(m_rootPath + "." + path, defaultVal);
	}
	std::string getString(
		const std::string& path,
		const std::string& defaultVal
	) const override {
		return m_parentCfg->getString(m_rootPath + "." + path, defaultVal);
	}
	std::vector<std::string> getStringArray(
		const std::string& path,
		const std::vector<std::string>& defaultVal
	) const override {
		return m_parentCfg->getStringArray(m_rootPath + "." + path, defaultVal);
	}
	Vector3 getVector3(const std::string& path, Vector3 defaultVal) const override {
		return m_parentCfg->getVector3(m_rootPath + "." + path, defaultVal);
	}
	nlohmann::json getSection(const std::string& path) const {
		return m_parentCfg->getSection(m_rootPath + "." + path);
	}
	SubGameConfig getSubConfig(const std::string& path) const {
		return m_parentCfg->getSubConfig(m_rootPath + "." + path);
	}

private:
	void setFloat(const std::string&, float) = delete;
	void setBool(const std::string&, bool) = delete;
	void setString(const std::string&, const std::string&) = delete;
	void setStringArray(
		const std::string&,
		const std::vector<std::string>&
	) = delete;
	void saveRoot(const std::string&) = delete;
	void saveChanged() = delete;
	void saveAll() = delete;
	void init(const std::vector<RootSource>&) = delete;
	void init(std::initializer_list<RootSource>) = delete;
	void initConstants() = delete;

	const GameConfig* m_parentCfg;
	std::string m_rootPath;
};

inline SubGameConfig GameConfig::getSubConfig(const std::string& path) const {
	return SubGameConfig(this, path);
}

#endif // GAME_CONFIG_HPP
