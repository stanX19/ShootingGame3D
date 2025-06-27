#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP
#include "components.hpp"
#include "game_context.hpp"

// utils
bool aimTargetExists(GameContext &context, AimTarget &target);

// Game systems
namespace ecs_systems
{
	void playerMoveControl(GameContext &context, float dt);
	void playerShootControl(GameContext &context);
	void playerAimTarget(GameContext &context);
	void enemyMoveControl(GameContext &context, float dt);
	void enemyAimTarget(GameContext &context);
	void enemyRespawn(GameContext &context);
	void entityMovement(GameContext &context, float dt);
	void entityCollision(GameContext &context, float dt);
	void entityAnchor(GameContext &context);
	void entityAnchorRelease(GameContext &context, float dt);
	void entityLifetime(GameContext &context, float dt);
	void delayedDamage(GameContext &context, float dt);
	void hpCleanup(GameContext &context);
	void hpRegen(GameContext &context, float dt);
	void ammoReload(GameContext &context, float dt);
	void bulletTargetAim(GameContext &context);
	void bulletWeaponShoot(GameContext &context);
	void weaponParentControlAim(GameContext &context);
	void weaponParentControlShoot(GameContext &context);
	void weaponUpdateCanFire(GameContext &context);
	void weaponUpdateFireStatus(GameContext &context);
	void weaponUpdateCooldown(GameContext &context, float dt);
	void asteroidRespawn(GameContext &context);
	void cleanOutOfBound(GameContext &context);
	void updatePlayerTargetable(GameContext &context);
	void syncModelRotation(GameContext &context); 
}

#endif // SYSTEMS_HPP