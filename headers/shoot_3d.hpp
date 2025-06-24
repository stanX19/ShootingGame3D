#ifndef SHOOT_3D_HPP
#define SHOOT_3D_HPP
#include "includes.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "constants.hpp"
#include "model_manager.hpp"
#include "game_context.hpp"
#include "utils.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>

void spawnBullet(GameContext &context, Position pos, Velocity velocity, HP hp,
				 Damage damage, float rad, Color color, Lifetime lifetime);
void spawnEnemy(GameContext &context, const Vector3 &pos);
void spawnFastEliteEnemy(GameContext &context, const Vector3 &pos);
void spawnEliteEnemy(GameContext &context, const Vector3 &pos);
void spawnDebris(GameContext &context, const Vector3 &position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f);
entt::entity spawnPlayer(GameContext &context);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad);

void emplaceWeaponMachineGun(GameContext &context, entt::entity entity);
void emplaceWeaponBasic(GameContext &context, entt::entity entity);
void emplaceWeaponSniper(GameContext &context, entt::entity entity);

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
	void entityLifetime(GameContext &context, float dt);
	void hpCleanup(GameContext &context);
	void hpRegen(GameContext &context, float dt);
	void ammoReload(GameContext &context, float dt);
	void bulletWeaponShoot(GameContext &context, float dt);
	void bulletTargetAim(GameContext &context);
	void asteroidRespawn(GameContext &context);
	void cleanOutOfBound(GameContext &context);
	void updatePlayerTargetable(GameContext &context);
	void model_rotation_sync(GameContext &context); 
}

#endif // SHOOT_3D_HPP