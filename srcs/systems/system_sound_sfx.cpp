#include "systems.hpp"

namespace
{
	float prevSpeed = 0.0f;
	float prevHp = 0.0f;
	float lowHpWarningDuration = 0.0f; // remaining duration to keep warning
	float lowHpWarningCooldown = 0.0f; // cooldown between beeps
	entt::entity lastLockOnTarget = entt::null;

	void lowHpWarningSfx(GameContext &context, float dt)
	{
		HP *hpPtr = context.registry.try_get<HP>(context.currentPlayer);
		if (!hpPtr)
			return;

		float lowHpThreshold = context.config.getFloat("sounds.lowHpWarningThreshold", 0.3f);
		bool isLowHp = hpPtr->value < hpPtr->maxValue * lowHpThreshold;
		bool tookDamage = hpPtr->value < prevHp;
		prevHp = hpPtr->value;

		if (isLowHp && tookDamage)
			lowHpWarningDuration = context.config.getFloat("sounds.lowHpWarningDuration", 10.0f);
		if (!isLowHp)
			lowHpWarningDuration = 0.0f;
		if (lowHpWarningDuration <= 0.0f)
			return;

		lowHpWarningDuration -= dt;
		lowHpWarningCooldown -= dt;
		if (lowHpWarningCooldown > 0.0f)
			return;
		lowHpWarningCooldown = context.config.getFloat("sounds.lowHpWarningInterval", 1.0f);
		
		float volume = context.config.getFloat("sounds.warningVolume", 1.0f);
		float fadeDuration = context.config.getFloat("sounds.lowHpWarningFadeDuration", 5.0f);
		if (lowHpWarningDuration <= fadeDuration)
			volume *= lowHpWarningDuration / fadeDuration;
		context.soundManager.playImmediate(context.config, "sounds.warning", volume);
	}

	void lockOnSfx(GameContext &context)
	{
		AimTarget *aimTargetPtr = context.registry.try_get<AimTarget>(context.currentPlayer);
		entt::entity targetedEntity = aimTargetPtr ? aimTargetPtr->entity : entt::null;
		if (targetedEntity != entt::null && targetedEntity != lastLockOnTarget) {
			float volume = context.config.getFloat("sounds.lockOnVolume", 1.0f);
			context.soundManager.playImmediate(context.config, "sounds.lockOn", volume);
		}
		lastLockOnTarget = targetedEntity;
	}
}

void ecs_systems::soundSfx(GameContext &context, float dt)
{
	// --- Player thrust sound ---
	Velocity *velPtr = context.registry.try_get<Velocity>(context.currentPlayer);
	float currentSpeed = velPtr ? Vector3Length(velPtr->value) : 0.0f;
	context.soundManager.updateThrustSound((currentSpeed - prevSpeed) / dt > 10.0f, dt);
	prevSpeed = currentSpeed;

	// --- Low HP warning sound ---
	lowHpWarningSfx(context, dt);

	// --- Lock-on sound ---
	lockOnSfx(context);
}
