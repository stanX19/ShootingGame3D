#include "systems.hpp"
#include "game_context.hpp"
#include "components/combat.hpp"
#include "components/weapon.hpp"
#include "components/physics.hpp"

void systems::SoundSfx::lowHpWarningSfx(GameContext &context, float dt)
{
	const HP *hpPtr = context.registry.try_get<HP>(context.currentPlayer);
	if (!hpPtr)
		return;

	const float lowHpThreshold = context.config.getFloat("sounds.lowHpWarningThreshold", 0.3f);
	const bool isLowHp = hpPtr->value < hpPtr->maxValue * lowHpThreshold;
	const bool tookDamage = hpPtr->value < m_prevHp;
	m_prevHp = hpPtr->value;

	if (isLowHp && tookDamage)
		m_lowHpWarningDuration = context.config.getFloat("sounds.lowHpWarningDuration", 10.0f);
	if (!isLowHp)
		m_lowHpWarningDuration = 0.0f;
	if (m_lowHpWarningDuration <= 0.0f)
		return;

	m_lowHpWarningDuration -= dt;
	m_lowHpWarningCooldown -= dt;
	if (m_lowHpWarningCooldown > 0.0f)
		return;
	m_lowHpWarningCooldown = context.config.getFloat("sounds.lowHpWarningInterval", 1.0f);
	
	float volume = context.config.getFloat("sounds.warningVolume", 1.0f);
	const float fadeDuration = context.config.getFloat("sounds.lowHpWarningFadeDuration", 5.0f);
	if (m_lowHpWarningDuration <= fadeDuration)
		volume *= m_lowHpWarningDuration / fadeDuration;
	context.soundManager.playImmediate(context.config, "sounds.warning", volume);
}

void systems::SoundSfx::lockOnSfx(GameContext &context)
{
	const AimTarget *aimTargetPtr = context.registry.try_get<AimTarget>(context.currentPlayer);
	const entt::entity targetedEntity = aimTargetPtr ? aimTargetPtr->entity : entt::null;
	if (targetedEntity != entt::null && targetedEntity != m_lastLockOnTarget) {
		const float volume = context.config.getFloat("sounds.lockOnVolume", 1.0f);
		context.soundManager.playImmediate(context.config, "sounds.lockOn", volume);
	}
	m_lastLockOnTarget = targetedEntity;
}

void systems::SoundSfx::update(GameContext &context, float dt)
{
	// --- Player thrust sound ---
	const Velocity *velPtr = context.registry.try_get<Velocity>(context.currentPlayer);
	const float currentSpeed = velPtr ? Vector3Length(velPtr->value) : 0.0f;
	context.soundManager.updateThrustSound((currentSpeed - m_prevSpeed) / dt > 10.0f, dt);
	m_prevSpeed = currentSpeed;

	// --- Low HP warning sound ---
	lowHpWarningSfx(context, dt);

	// --- Lock-on sound ---
	lockOnSfx(context);
}

