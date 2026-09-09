#include "classes/hud_manager.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/collision.hpp"
#include "components/combat.hpp"
#include "components/movement.hpp"
#include "components/unit.hpp"
#include "utils/algorithm_utils.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr size_t MAX_DAMAGE_NUMBERS = 64;
	constexpr size_t MAX_LEFT_LOG_ENTRIES = 5;
	constexpr float FLOAT_SPEED = 3.5f;

	Vector3 computeTargetOffset(const entt::registry &registry, entt::entity target, Vector3 hitPos) {
		if (target == entt::null || !registry.valid(target) || !registry.all_of<Position>(target))
			return Vector3Zeros;
		const Vector3 targetPos = registry.get<Position>(target).value;
		return Vector3Subtract(hitPos, targetPos);
	}

	void updateDamageNumberPosition(HudManager::ActiveDamageNumber &dmg, float dt, const entt::registry &registry) {
		dmg.timer += dt;
		dmg.offset.y += FLOAT_SPEED * dt;
		dmg.scale = std::max(1.0f, dmg.scale - 3.0f * dt);

		if (dmg.target != entt::null && registry.valid(dmg.target) && registry.all_of<Position>(dmg.target)) {
			const Vector3 targetPos = registry.get<Position>(dmg.target).value;
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
	m_damageRequests.push_back({attacker, target, amount, hitPos, hitType});
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

void HudManager::reset() {
	m_damageRequests.clear();
	m_toastRequests.clear();
	m_damageNumbers.clear();
	m_toasts.clear();
	m_observedEntity = entt::null;
	m_collisionAlertCooldown = 0.0f;
	m_collisionBlinkTimer = 0.0f;
	m_collisionAlpha = 0.0f;
	m_collisionWarnings.clear();
	m_missileAlertCooldown = 0.0f;
	m_missileBlinkTimer = 0.0f;
	m_missileAlpha = 0.0f;
	m_missileWarnings.clear();
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
			existing.offset = computeTargetOffset(context.registry, req.target, req.hitPos);

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
		newDmg.offset = computeTargetOffset(context.registry, req.target, req.hitPos);
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

void HudManager::updateCollisionAlerts(float dt, GameContext &context) {
	const static float warningTime = 5.0f;
	const static float warningDist = 10.0f;
	m_collisionWarnings.clear();

	m_collisionAlertCooldown -= dt;
	bool canPlayAlert = m_collisionAlertCooldown <= 0.0f;
	if (m_collisionAlertCooldown <= 0.0f) {
		m_collisionAlertCooldown = 1.0f;
	}

	if (!context.registry.valid(context.currentPlayer))
		return;

	auto [posA, velA, bodyA] = context.registry.try_get<Position, Velocity, CollisionBody>(context.currentPlayer);
	if (!posA || !velA || !bodyA)
		return;

	for (auto [other, posB, bodyB, dmgB] : context.registry.view<Position, CollisionBody, Damage, tag::Asteroid>(entt::exclude<tag::Bullet>).each()) {
		if (context.currentPlayer == other)
			continue;
		Velocity velB = context.registry.all_of<Velocity>(other) ? context.registry.get<Velocity>(other) : Velocity{Vector3Zeros};
		if (willCollide(posA->value, velA->value, posB.value, velB.value, bodyA->radius + bodyB.radius + warningDist, warningTime)) {
			m_collisionWarnings.push_back({posB.value, Vector3Distance(posA->value, posB.value) - bodyA->radius - bodyB.radius});
			if (canPlayAlert)
				context.soundManager.queueSound(context.config, "sounds.collisionAlert", posB.value, 0.5f);
		}
	}

	if (m_collisionWarnings.empty())
		return;

	m_collisionBlinkTimer += dt * 6.0f;
	m_collisionAlpha = 0.7f + 0.3f * std::sin(m_collisionBlinkTimer);
}

void HudManager::updateMissileAlerts(float dt, GameContext &context) {
	const static float warningTime = 5.0f;
	m_missileWarnings.clear();

	m_missileAlertCooldown -= dt;
	bool canPlayAlert = m_missileAlertCooldown <= 0.0f;
	if (m_missileAlertCooldown <= 0.0f) {
		m_missileAlertCooldown = 1.0f;
	}

	if (!context.registry.valid(context.currentPlayer))
		return;

	auto [posA, velA, bodyA] = context.registry.try_get<Position, Velocity, CollisionBody>(context.currentPlayer);
	if (!posA || !velA || !bodyA)
		return;

	for (auto [other, posB, bodyB, dmgB, velB, target] : context.registry.view<Position, CollisionBody, Damage, Velocity, MoveTarget, tag::Missile>().each()) {
		if (context.currentPlayer == other || target.entity != context.currentPlayer)
			continue;
		float distance = Vector3Distance(posA->value, posB.value);
		if (warningTime * Vector3Length(velB.value - velA->value) < distance)
			continue;
		bool willCollideFlag = Vector3DotProduct(posA->value - posB.value, velB.value - velA->value) > 0;
		if (willCollideFlag) {
			m_missileWarnings.push_back({posB.value, distance - bodyA->radius - bodyB.radius});
			if (canPlayAlert)
				context.soundManager.queueSound(context.config, "sounds.missileAlert", posB.value, 0.5f);
		}
	}

	if (m_missileWarnings.empty())
		return;

	m_missileBlinkTimer += dt * 6.0f;
	m_missileAlpha = 0.7f + 0.3f * std::sin(m_missileBlinkTimer);
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
	updateCollisionAlerts(dt, context);
	updateMissileAlerts(dt, context);
	updateActiveDamageNumbers(dt, context);
	updateActiveToasts(dt);
}
