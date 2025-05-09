#ifndef SHOOT_3D_HPP
#define SHOOT_3D_HPP
#include "includes.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>

#define ARENA_SIZE 200.0f

// Utility functions
Vector3 GetForwardVector(const Rotation& rotation);
Vector3 GetRightVector(const Rotation& rotation);
Vector3 GetUpVector();
Vector3 GetUpVector(const Rotation& rotation);

void spawBullet(entt::registry& registry, const Vector3& pos, const Vector3& dir, float damage, Color color);
void spawnEnemy(entt::registry& registry, const Vector3& pos);
void spawnFastEliteEnemy(entt::registry& registry, const Vector3& pos);
void spawnEliteEnemy(entt::registry& registry, const Vector3& pos);
void spawnDebris(entt::registry& registry, const Vector3& position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f);
entt::entity spawnPlayer(entt::registry& registry);
void spawnAsteroid(entt::registry& registry, const Vector3& pos, const Vector3& dir);
void spawnAsteroid(entt::registry& registry, const Vector3& pos, const Vector3& dir, float rad);

void emplaceWeaponMachineGun(entt::registry& registry, entt::entity entity);
void emplaceWeaponBasic(entt::registry& registry, entt::entity entity);

// Game systems
namespace ecs_systems {
	void playerMoveControl(entt::registry& registry, float dt);
	void playerShootControl(entt::registry &registry);
	void playerAimTarget(entt::registry& registry);
	void enemyMoveControl(entt::registry& registry, float dt);
	void enemyAimTarget(entt::registry& registry);
	void enemyRespawn(entt::registry& registry);
	void enemyMovement(entt::registry& registry, float dt);
	void entityCollision(entt::registry& registry);
	void entityLifetime(entt::registry& registry, float dt);
	void hpCleanup(entt::registry& registry);
	void hpRegen(entt::registry& registry, float dt);
	void ammoReload(entt::registry& registry, float dt);
	void bulletWeaponShoot(entt::registry& registry, float dt);
	void bulletTargetAim(entt::registry &registry);
	void asteroidRespawn(entt::registry &registry);
	void cleanOutOfBound(entt::registry &registry);
	void updatePlayerTargetable(entt::registry &registry);	
}

#endif // SHOOT_3D_HPP