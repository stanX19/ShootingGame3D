#include "systems.hpp"

namespace
{
	float prevSpeed = 0.0f;
	float prevHp = 0.0f;
	float lowHpWarningDuration = 0.0f; // remaining duration to keep warning
	float lowHpWarningCooldown = 0.0f; // cooldown between beeps

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
			lowHpWarningDuration = context.config.getFloat("sounds.lowHpWarningDuration", 3.0f);
		if (!isLowHp)
			lowHpWarningDuration = 0.0f;
		if (lowHpWarningDuration <= 0.0f)
			return;

		lowHpWarningDuration -= dt;
		lowHpWarningCooldown -= dt;
		if (lowHpWarningCooldown > 0.0f)
			return;

		lowHpWarningCooldown = context.config.getFloat("sounds.lowHpWarningInterval", 1.0f);
		sound::Id warningId = context.soundManager.getConfigSound(context.config, "sounds.warning");
		context.soundManager.playImmediate(warningId, context.config.getFloat("sounds.warningVolume", 1.0f));
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
}
