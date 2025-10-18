#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP
#include "components.hpp"
#include "game_context.hpp"

// utils
bool aimTargetExists(GameContext &context, AimTarget &target);

// Game systems
namespace ecs_systems
{
	void playerMoveControl(GameContext &context, float dt, const Camera3D &camera);
	void playerShootControl(GameContext &context);
	void playerRespawn(GameContext &context);
	void aiMoveControl(GameContext &context, float dt);
	void aiShootControl(GameContext &context);
	void aiFindTarget(GameContext &context);
	void blueUnitRespawn(GameContext &context);
	void redUnitRespawn(GameContext &context);
	void entityMovement(GameContext &context, float dt);
	void entityTransformation(GameContext &context, float dt);
	void detectEntityCollision(GameContext &context, float dt);
	void entityAnchor(GameContext &context, float dt);
	void entityAnchorRelease(GameContext &context, float dt);
	void entityLifetime(GameContext &context, float dt);
	void delayedDamage(GameContext &context, float dt);
	void hpCleanup(GameContext &context);
	void hpRegen(GameContext &context, float dt);
	void energyShield(GameContext &context, float dt);
	void ammoReload(GameContext &context, float dt);
	void bulletTargetAim(GameContext &context);
	void weaponShoot(GameContext &context);
	void weaponParentControlAim(GameContext &context);
	void weaponParentControlShoot(GameContext &context);
	void weaponUpdateCanFire(GameContext &context);
	void weaponUpdateFireStatus(GameContext &context);
	void weaponUpdateCooldown(GameContext &context, float dt);
	void asteroidRespawn(GameContext &context);
	void cleanOutOfBound(GameContext &context);
	void syncModelRotation(GameContext &context);
	void spawnTrailParticles(GameContext &context, float dt);
}

#endif // SYSTEMS_HPP