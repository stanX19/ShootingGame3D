#include "systems.hpp"
#include "game_context.hpp"
#include "components/combat.hpp"
#include "components/weapon.hpp"
#include "components/physics.hpp"

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

	// --- Lock-on sound ---
	lockOnSfx(context);
}

