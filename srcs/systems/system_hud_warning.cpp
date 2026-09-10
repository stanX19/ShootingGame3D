#include "systems.hpp"
#include "game_context.hpp"
#include "components/combat.hpp"
#include "components/physics.hpp"
#include "components/collision.hpp"
#include "components/movement.hpp"
#include "components/unit.hpp"
#include "utils/algorithm_utils.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>

void systems::HudWarning::update(GameContext &context, const float dt)
{
	const entt::entity observed = context.hudManager.getObservedEntity();
	const entt::entity targetEntity = (observed != entt::null) ? observed : context.currentPlayer;

	if (targetEntity == entt::null || !context.registry.valid(targetEntity)) {
		context.hudManager.clearWarnings();
		context.hudManager.clearWarningToasts();
		return;
	}

	const bool lowHpActive = updateLowHpWarning(context, dt, targetEntity);
	const bool collisionActive = updateCollisionWarning(context, dt, targetEntity);
	const bool missileActive = updateMissileWarning(context, dt, targetEntity);

	std::vector<HudManager::WarningAlert> alerts;
	if (collisionActive)
		alerts.push_back({"PROXIMITY ALERT", context.hudManager.getCollisionAlertAlpha()});
	if (missileActive)
		alerts.push_back({"MISSILE ALERT", context.hudManager.getMissileAlertAlpha()});
	if (lowHpActive) {
		const float lowHpAlpha = 0.7f + 0.3f * std::sin(m_lowHpBlinkTimer);
		alerts.push_back({"LOW HP", lowHpAlpha});
	}

	context.hudManager.setWarningAlerts(alerts);
}

bool systems::HudWarning::updateLowHpWarning(GameContext &context, const float dt, const entt::entity targetEntity)
{
	const HP *hpPtr = context.registry.try_get<HP>(targetEntity);
	if (!hpPtr)
		return false;

	const float lowHpThreshold = context.config.getFloat("sounds.lowHpWarningThreshold", 0.3f);
	const bool isLowHp = hpPtr->value < hpPtr->maxValue * lowHpThreshold;
	const bool tookDamage = hpPtr->value < m_prevHp;
	m_prevHp = hpPtr->value;

	if (isLowHp && tookDamage)
		m_lowHpWarningDuration = context.config.getFloat("sounds.lowHpWarningDuration", 10.0f);
	if (!isLowHp) {
		m_lowHpWarningDuration = 0.0f;
		return false;
	}

	if (m_lowHpWarningDuration > 0.0f) {
		m_lowHpWarningDuration -= dt;
		m_lowHpWarningCooldown -= dt;
		if (m_lowHpWarningCooldown <= 0.0f) {
			m_lowHpWarningCooldown = context.config.getFloat("sounds.lowHpWarningInterval", 1.0f);
			float volume = context.config.getFloat("sounds.warningVolume", 1.0f);
			const float fadeDuration = context.config.getFloat("sounds.lowHpWarningFadeDuration", 5.0f);
			if (m_lowHpWarningDuration <= fadeDuration)
				volume *= m_lowHpWarningDuration / fadeDuration;
			context.soundManager.playImmediate(context.config, "sounds.warning", volume);
		}
	}

	m_lowHpBlinkTimer += dt * 6.0f;
	return true;
}

bool systems::HudWarning::updateCollisionWarning(GameContext &context, const float dt, const entt::entity targetEntity)
{
	constexpr float warningTime = 5.0f;
	constexpr float warningDist = 10.0f;

	m_collisionAlertCooldown -= dt;
	const bool canPlayAlert = m_collisionAlertCooldown <= 0.0f;
	if (canPlayAlert)
		m_collisionAlertCooldown = 1.0f;

	const auto [posA, velA, bodyA] = context.registry.try_get<Position, Velocity, CollisionBody>(targetEntity);
	if (!posA || !velA || !bodyA) {
		context.hudManager.setCollisionWarnings({}, 0.0f);
		return false;
	}

	std::vector<std::pair<Vector3, float>> warnings;
	for (const auto [other, posB, bodyB, dmgB] : context.registry.view<Position, CollisionBody, Damage, tag::Asteroid>(entt::exclude<tag::Bullet>).each()) {
		if (targetEntity == other)
			continue;
		const Velocity velB = context.registry.all_of<Velocity>(other) ? context.registry.get<Velocity>(other) : Velocity{Vector3Zeros};
		if (!willCollide(posA->value, velA->value, posB.value, velB.value, bodyA->radius + bodyB.radius + warningDist, warningTime))
			continue;

		const float dist = Vector3Distance(posA->value, posB.value) - bodyA->radius - bodyB.radius;
		warnings.push_back({posB.value, dist});
		if (canPlayAlert)
			context.soundManager.queueSound(context.config, "sounds.collisionAlert", posB.value, 0.5f);
	}

	if (warnings.empty()) {
		context.hudManager.setCollisionWarnings({}, 0.0f);
		return false;
	}

	m_collisionBlinkTimer += dt * 6.0f;
	const float alpha = 0.7f + 0.3f * std::sin(m_collisionBlinkTimer);
	context.hudManager.setCollisionWarnings(std::move(warnings), alpha);
	return true;
}

bool systems::HudWarning::updateMissileWarning(GameContext &context, const float dt, const entt::entity targetEntity)
{
	constexpr float warningTime = 5.0f;

	m_missileAlertCooldown -= dt;
	const bool canPlayAlert = m_missileAlertCooldown <= 0.0f;
	if (canPlayAlert)
		m_missileAlertCooldown = 1.0f;

	const auto [posA, velA, bodyA] = context.registry.try_get<Position, Velocity, CollisionBody>(targetEntity);
	if (!posA || !velA || !bodyA) {
		context.hudManager.setMissileWarnings({}, 0.0f);
		return false;
	}

	std::vector<std::pair<Vector3, float>> warnings;
	for (const auto [other, posB, bodyB, dmgB, velB, target] : context.registry.view<Position, CollisionBody, Damage, Velocity, MoveTarget, tag::Missile>().each()) {
		if (targetEntity == other || target.entity != targetEntity)
			continue;

		const float distance = Vector3Distance(posA->value, posB.value);
		if (warningTime * Vector3Length(velB.value - velA->value) < distance)
			continue;

		const bool willCollideFlag = Vector3DotProduct(posA->value - posB.value, velB.value - velA->value) > 0.0f;
		if (!willCollideFlag)
			continue;

		warnings.push_back({posB.value, distance - bodyA->radius - bodyB.radius});
		if (canPlayAlert)
			context.soundManager.queueSound(context.config, "sounds.missileAlert", posB.value, 0.5f);
	}

	if (warnings.empty()) {
		context.hudManager.setMissileWarnings({}, 0.0f);
		return false;
	}

	m_missileBlinkTimer += dt * 6.0f;
	const float alpha = 0.7f + 0.3f * std::sin(m_missileBlinkTimer);
	context.hudManager.setMissileWarnings(std::move(warnings), alpha);
	return true;
}
