#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP
#include "components.hpp"
#include "game_context.hpp"

// utils
bool aimTargetExists(GameContext &context, AimTarget &target);

// Game systems
namespace ecs_systems
{
	void cameraFollowPlayer(GameContext &context, float dt);
	void playerMoveControl(GameContext &context, float dt);
	void playerShootControl(GameContext &context, float dt);
	void playerRespawn(GameContext &context, float dt);
	void aiMoveControl(GameContext &context, float dt);
	void aiShootControl(GameContext &context, float dt);
	void aiFindTarget(GameContext &context, float dt);
	void blueUnitRespawn(GameContext &context, float dt);
	void redUnitRespawn(GameContext &context, float dt);
	void entityMovement(GameContext &context, float dt);
	void applyImpulse(GameContext &context, float dt);
	void entityTransformation(GameContext &context, float dt);
	void detectEntityCollision(GameContext &context, float dt);
	void entityAnchor(GameContext &context, float dt);
	void entityAnchorRelease(GameContext &context, float dt);
	void entityLifetime(GameContext &context, float dt);
	void delayedDamage(GameContext &context, float dt);
	void hpCleanup(GameContext &context, float dt);
	void hpRegen(GameContext &context, float dt);
	void energyShield(GameContext &context, float dt);
	void ammoReload(GameContext &context, float dt);
	void bulletTargetAim(GameContext &context, float dt);
	void weaponShoot(GameContext &context, float dt);
	void weaponParentControlAim(GameContext &context, float dt);
	void weaponParentControlShoot(GameContext &context, float dt);
	void weaponUpdateCanFire(GameContext &context, float dt);
	void weaponUpdateFireStatus(GameContext &context, float dt);
	void weaponUpdateCooldown(GameContext &context, float dt);
	void weaponUpdateCharged(GameContext &context, float dt);
	void asteroidRespawn(GameContext &context, float dt);
	void cleanOutOfBound(GameContext &context, float dt);
	void syncModelRotation(GameContext &context, float dt);
	void spawnTrailParticles(GameContext &context, float dt);
	void soundSfx(GameContext &context, float dt);
}

#endif // SYSTEMS_HPP