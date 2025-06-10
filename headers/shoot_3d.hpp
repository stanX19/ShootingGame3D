#ifndef SHOOT_3D_HPP
#define SHOOT_3D_HPP
#include "includes.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "constants.hpp"
#include "mesh_manager.hpp"
#include "utils.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>

struct GameContext {
	MeshManager meshManager;
	entt::registry &registry;
};

void spawnBullet(entt::registry &registry, Position pos, Velocity velocity, HP hp,
				 Damage damage, float rad, Color color, Lifetime lifetime);
void spawnEnemy(entt::registry &registry, const Vector3 &pos);
void spawnFastEliteEnemy(entt::registry &registry, const Vector3 &pos);
void spawnEliteEnemy(entt::registry &registry, const Vector3 &pos);
void spawnDebris(entt::registry &registry, const Vector3 &position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f);
entt::entity spawnPlayer(entt::registry &registry);
void spawnAsteroid(entt::registry &registry, const Vector3 &pos, const Vector3 &dir);
void spawnAsteroid(entt::registry &registry, const Vector3 &pos, const Vector3 &dir, float rad);

void emplaceWeaponMachineGun(entt::registry &registry, entt::entity entity);
void emplaceWeaponBasic(entt::registry &registry, entt::entity entity);
void emplaceWeaponSniper(entt::registry &registry, entt::entity entity);

// Game systems
namespace ecs_systems
{
	void playerMoveControl(entt::registry &registry, float dt);
	void playerShootControl(entt::registry &registry);
	void playerAimTarget(entt::registry &registry);
	void enemyMoveControl(entt::registry &registry, float dt);
	void enemyAimTarget(entt::registry &registry);
	void enemyRespawn(entt::registry &registry);
	void entityMovement(entt::registry &registry, float dt);
	void entityCollision(entt::registry &registry, float dt);
	void entityLifetime(entt::registry &registry, float dt);
	void hpCleanup(entt::registry &registry);
	void hpRegen(entt::registry &registry, float dt);
	void ammoReload(entt::registry &registry, float dt);
	void bulletWeaponShoot(entt::registry &registry, float dt);
	void bulletTargetAim(entt::registry &registry);
	void asteroidRespawn(entt::registry &registry);
	void cleanOutOfBound(entt::registry &registry);
	void updatePlayerTargetable(entt::registry &registry);
}

#endif // SHOOT_3D_HPP