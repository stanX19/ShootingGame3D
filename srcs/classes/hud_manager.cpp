#include "classes/hud_manager.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/collision.hpp"
#include "components/combat.hpp"
#include "components/spaceship.hpp"
#include "components/identity.hpp"
#include "utils/algorithm_utils.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr size_t MAX_DAMAGE_NUMBERS = 64;
	constexpr size_t MAX_LEFT_LOG_ENTRIES = 5;
	constexpr float FLOAT_SPEED = 3.5f;

	Vector3 computeTargetOffset(const entt::registry &registry, entt::entity target, Vector3 hitPos) {
		if (target == entt::null || !registry.valid(target) || !registry.all_of<physics::Position>(target))
			return Vector3Zeros;
		const Vector3 targetPos = registry.get<physics::Position>(target).value;
		return Vector3Subtract(hitPos, targetPos);
	}

	void updateDamageNumberPosition(HudManager::ActiveDamageNumber &dmg, float dt, const entt::registry &registry) {
		dmg.timer += dt;
		dmg.offset.y += FLOAT_SPEED * dt;
		dmg.scale = std::max(1.0f, dmg.scale - 3.0f * dt);

		if (dmg.target != entt::null && registry.valid(dmg.target) && registry.all_of<physics::Position>(dmg.target)) {
			const Vector3 targetPos = registry.get<physics::Position>(dmg.target).value;
			dmg.worldPos = Vector3Add(targetPos, dmg.offset);
			return;
		}
		dmg.worldPos.y += FLOAT_SPEED * dt;
	}
}

HudManager::HudManager() {
	m_damageRequests.reserve(32);
	m_toastRequests.reserve(16);
	m_damageNumbers.reserve(MAX_DAMAGE_NUMBERS);
	m_toasts.reserve(16);
}

void HudManager::setObservedEntity(entt::entity entity) {
	m_observedEntity = entity;
}

entt::entity HudManager::getObservedEntity() const {
	return m_observedEntity;
}

void HudManager::reportDamage(entt::entity attacker, entt::entity target, float amount, Vector3 hitPos, HitType hitType) {
	reportDamage(attacker, target, amount, hitPos, Vector3Zeros, hitType);
}

void HudManager::reportDamage(entt::entity attacker, entt::entity target, float amount, Vector3 hitPos, Vector3 targetOffset, HitType hitType) {
	m_damageRequests.push_back({attacker, target, amount, hitPos, targetOffset, hitType});
}

void HudManager::addToast(const ToastConfig &config) {
	m_toastRequests.push_back(config);
}

void HudManager::addToastLeftLog(const std::string &message, Color color) {
	ToastConfig config;
	config.text = message;
	config.color = color;
	config.fontSize = 18;
	config.duration = 4.0f;
	config.slot = ToastSlot::LEFT_LOG;
	config.priority = ToastPriority::NORMAL;
	m_toastRequests.push_back(config);
}

void HudManager::addToastTopNotif(const std::string &message, ToastPriority priority, Color color) {
	ToastConfig config;
	config.text = message;
	config.color = color;
	config.fontSize = 26;
	config.duration = 3.0f;
	config.slot = ToastSlot::TOP_NOTIF;
	config.priority = priority;
	m_toastRequests.push_back(config);
}

void HudManager::assignFirstFreeSlot(const std::string &text) {
	for (auto &slot : m_warningSlots) {
		if (slot.text.empty()) {
			slot.text = text;
			return;
		}
	}
}

void HudManager::rebuildWarningToasts(const std::vector<WarningAlert> &alerts) {
	std::erase_if(m_toasts, [](const ActiveToast &t) {
		return t.slot >= ToastSlot::WARNING_TOP;
	});
	for (const auto &slot : m_warningSlots) {
		if (slot.text.empty())
			continue;
		for (const auto &alert : alerts) {
			if (alert.text != slot.text)
				continue;
			ActiveToast newToast;
			newToast.text = alert.text;
			newToast.color = ColorAlpha(RED, alert.alpha);
			newToast.timer = 0.0f;
			newToast.maxDuration = 0.0f;
			newToast.priority = ToastPriority::CRITICAL;
			newToast.slot = slot.slot;
			m_toasts.push_back(std::move(newToast));
			break;
		}
	}
}

void HudManager::setWarningAlerts(const std::vector<WarningAlert> &alerts) {
	for (auto &slot : m_warningSlots) {
		if (slot.text.empty())
			continue;
		const bool active = std::any_of(alerts.begin(), alerts.end(), [&](const auto &a) {
			return a.text == slot.text;
		});
		if (!active)
			slot.text.clear();
	}
	for (const auto &alert : alerts) {
		const bool assigned = std::any_of(std::begin(m_warningSlots), std::end(m_warningSlots), [&](const auto &s) {
			return s.text == alert.text;
		});
		if (!assigned)
			assignFirstFreeSlot(alert.text);
	}
	rebuildWarningToasts(alerts);
}

void HudManager::clearWarningToasts() {
	std::erase_if(m_toasts, [](const ActiveToast &t) {
		return t.slot >= ToastSlot::WARNING_TOP;
	});
	for (auto &slot : m_warningSlots) {
		slot.text.clear();
	}
}

void HudManager::reset() {
	m_damageRequests.clear();
	m_toastRequests.clear();
	m_damageNumbers.clear();
	m_toasts.clear();
	m_observedEntity = entt::null;
	clearWarnings();
	clearWarningToasts();
}

size_t HudManager::getActiveDamageNumberCount() const {
	return m_damageNumbers.size();
}

size_t HudManager::getActiveToastCount() const {
	return m_toasts.size();
}

void HudManager::processDamageRequests(GameContext &context) {
	for (const auto &req : m_damageRequests) {
		if (m_observedEntity != entt::null && req.attacker != m_observedEntity)
			continue;

		bool aggregated = false;
		for (auto &existing : m_damageNumbers) {
			if (existing.target == entt::null || existing.target != req.target)
				continue;
			if (existing.timer >= context.config.hud.damageNumbers.resetCooldown)
				continue;

			existing.totalDamage += req.amount;
			existing.timer = 0.0f;
			existing.scale = std::min(existing.scale + 0.05f, 1.25f);
			existing.worldPos = req.hitPos;
			existing.offset = (Vector3LengthSqr(req.targetOffset) > 0.0f)
				? req.targetOffset
				: computeTargetOffset(context.registry, req.target, req.hitPos);

			if (req.hitType == HitType::KILL) {
				existing.hitType = HitType::KILL;
			} else if (req.hitType == HitType::CRITICAL && existing.hitType == HitType::NORMAL) {
				existing.hitType = HitType::CRITICAL;
			}

			aggregated = true;
			break;
		}

		if (aggregated)
			continue;

		// Remove previous damage numbers for this target so old text disappears
		// immediately instead of slowly fading away when the moving target gets a new text
		if (req.target != entt::null) {
			std::erase_if(m_damageNumbers, [target = req.target](const auto &d) {
				return d.target == target;
			});
		}

		if (m_damageNumbers.size() >= MAX_DAMAGE_NUMBERS)
			m_damageNumbers.erase(m_damageNumbers.begin());

		ActiveDamageNumber newDmg;
		newDmg.target = req.target;
		newDmg.worldPos = req.hitPos;
		newDmg.offset = (Vector3LengthSqr(req.targetOffset) > 0.0f)
			? req.targetOffset
			: computeTargetOffset(context.registry, req.target, req.hitPos);
		newDmg.totalDamage = req.amount;
		newDmg.timer = 0.0f;
		newDmg.maxDuration = 1.0f;
		newDmg.scale = 1.15f;
		newDmg.hitType = req.hitType;
		m_damageNumbers.push_back(newDmg);
	}
	m_damageRequests.clear();
}

void HudManager::processToastRequests() {
	for (const auto &req : m_toastRequests) {
		if (req.slot == ToastSlot::LEFT_LOG) {
			size_t logCount = 0;
			for (const auto &t : m_toasts) {
				if (t.slot == ToastSlot::LEFT_LOG)
					logCount++;
			}
			if (logCount >= MAX_LEFT_LOG_ENTRIES) {
				for (auto it = m_toasts.begin(); it != m_toasts.end(); ++it) {
					if (it->slot == ToastSlot::LEFT_LOG) {
						m_toasts.erase(it);
						break;
					}
				}
			}
		} else if (req.slot == ToastSlot::TOP_NOTIF) {
			for (auto it = m_toasts.begin(); it != m_toasts.end();) {
				if (it->slot == ToastSlot::TOP_NOTIF && it->priority <= req.priority) {
					it = m_toasts.erase(it);
				} else {
					++it;
				}
			}
		}

		ActiveToast newToast;
		newToast.text = req.text;
		newToast.screenPos = req.screenPos;
		newToast.fontSize = req.fontSize;
		newToast.color = req.color;
		newToast.timer = 0.0f;
		newToast.maxDuration = req.duration;
		newToast.priority = req.priority;
		newToast.slot = req.slot;
		m_toasts.push_back(newToast);
	}
	m_toastRequests.clear();
}

void HudManager::updateActiveDamageNumbers(float dt, GameContext &context) {
	for (auto it = m_damageNumbers.begin(); it != m_damageNumbers.end();) {
		updateDamageNumberPosition(*it, dt, context.registry);
		if (it->timer >= it->maxDuration) {
			it = m_damageNumbers.erase(it);
			continue;
		}
		++it;
	}
}

void HudManager::updateActiveToasts(float dt) {
	for (auto it = m_toasts.begin(); it != m_toasts.end();) {
		if (it->maxDuration <= 0.0f) {
			++it;
			continue;
		}
		it->timer += dt;
		if (it->timer >= it->maxDuration) {
			it = m_toasts.erase(it);
			continue;
		}
		++it;
	}
}

void HudManager::update(float dt, GameContext &context) {
	processDamageRequests(context);
	processToastRequests();
	updateActiveDamageNumbers(dt, context);
	updateActiveToasts(dt);
}
