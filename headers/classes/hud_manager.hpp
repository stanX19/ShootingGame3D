#pragma once

#include "includes.hpp"
#include <vector>
#include <string>
#include <utility>

class GameContext;
class GameConfig;

enum class HitType {
	NORMAL = 0,
	CRITICAL,
	KILL
};

inline Color hitTypeToColor(HitType type) {
	if (type == HitType::KILL)
		return RED;
	if (type == HitType::CRITICAL)
		return ORANGE;
	return WHITE;
}

enum class ToastPriority {
	LOW = 0,
	NORMAL,
	HIGH,
	CRITICAL
};

enum class ToastSlot {
	FREEFORM = 0,
	LEFT_LOG,
	TOP_NOTIF
};

struct ToastConfig {
	std::string text;
	Vector2 screenPos{0.0f, 0.0f};
	int fontSize = 20;
	Color color = WHITE;
	float duration = 3.0f;
	ToastPriority priority = ToastPriority::NORMAL;
	ToastSlot slot = ToastSlot::FREEFORM;
};

struct DamageReport {
	entt::entity attacker = entt::null;
	entt::entity target = entt::null;
	float amount = 0.0f;
	Vector3 hitPos{0.0f, 0.0f, 0.0f};
	HitType hitType = HitType::NORMAL;
};

class HudManager {
public:
	HudManager();
	~HudManager() = default;

	void setObservedEntity(entt::entity entity);
	entt::entity getObservedEntity() const;

	void reportDamage(entt::entity attacker, entt::entity target, float amount, Vector3 hitPos, HitType hitType = HitType::NORMAL);
	void addToast(const ToastConfig &config);
	void addToastLeftLog(const std::string &message, Color color = WHITE);
	void addToastTopNotif(const std::string &message, ToastPriority priority = ToastPriority::HIGH, Color color = YELLOW);

	void update(float dt, GameContext &context);
	void reset();

	size_t getActiveDamageNumberCount() const;
	size_t getActiveToastCount() const;

	struct ActiveDamageNumber {
		entt::entity target = entt::null;
		Vector3 worldPos{0.0f, 0.0f, 0.0f};
		Vector3 offset{0.0f, 0.0f, 0.0f};
		float totalDamage = 0.0f;
		float timer = 0.0f;
		float maxDuration = 1.0f;
		float scale = 1.0f;
		HitType hitType = HitType::NORMAL;
	};

	struct ActiveToast {
		std::string text;
		Vector2 screenPos{0.0f, 0.0f};
		int fontSize = 20;
		Color color = WHITE;
		float timer = 0.0f;
		float maxDuration = 3.0f;
		ToastPriority priority = ToastPriority::NORMAL;
		ToastSlot slot = ToastSlot::FREEFORM;
	};

	const std::vector<ActiveDamageNumber>& getActiveDamageNumbers() const { return m_damageNumbers; }
	const std::vector<ActiveToast>& getActiveToasts() const { return m_toasts; }

	bool hasCollisionWarnings() const { return !m_collisionWarnings.empty(); }
	float getCollisionAlertAlpha() const { return m_collisionAlpha; }
	const std::vector<std::pair<Vector3, float>>& getCollisionWarnings() const { return m_collisionWarnings; }

	bool hasMissileWarnings() const { return !m_missileWarnings.empty(); }
	float getMissileAlertAlpha() const { return m_missileAlpha; }
	const std::vector<std::pair<Vector3, float>>& getMissileWarnings() const { return m_missileWarnings; }

private:
	entt::entity m_observedEntity = entt::null;
	std::vector<DamageReport> m_damageRequests;
	std::vector<ToastConfig> m_toastRequests;
	std::vector<ActiveDamageNumber> m_damageNumbers;
	std::vector<ActiveToast> m_toasts;

	float m_collisionAlertCooldown = 0.0f;
	float m_collisionBlinkTimer = 0.0f;
	float m_collisionAlpha = 0.0f;
	std::vector<std::pair<Vector3, float>> m_collisionWarnings;

	float m_missileAlertCooldown = 0.0f;
	float m_missileBlinkTimer = 0.0f;
	float m_missileAlpha = 0.0f;
	std::vector<std::pair<Vector3, float>> m_missileWarnings;

	void processDamageRequests(GameContext &context);
	void processToastRequests();
	void updateCollisionAlerts(float dt, GameContext &context);
	void updateMissileAlerts(float dt, GameContext &context);
	void updateActiveDamageNumbers(float dt, GameContext &context);
	void updateActiveToasts(float dt);
};
