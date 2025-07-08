#ifndef ENTITIES_HPP
#define ENTITIES_HPP
#include "includes.hpp"
#include "components.hpp"
#include "game_context.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>

entt::entity spawnEnemy(GameContext &context, const Vector3 &pos);
entt::entity spawnFastEliteEnemy(GameContext &context, const Vector3 &pos);
entt::entity spawnEliteEnemy(GameContext &context, const Vector3 &pos);
entt::entity spawnMothershipEnemy(GameContext &context, const Vector3& pos);
entt::entity spawnPlayer(GameContext &context);
entt::entity spawnUnlinkedAutoTurret(GameContext &context, Color color);
entt::entity spawnLinkedTurret(GameContext &context, Color color, entt::entity &parent, Vector3 relpos);
entt::entity spawnLinkedAutoTurret(GameContext &context, Color color, entt::entity &parent, Vector3 relpos);
void spawnBullet(GameContext &context, Position pos, Velocity velocity, HP hp,
				 Damage damage, float rad, Color color, Lifespan lifetime);
void spawnDebris(GameContext &context, const Vector3 &position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad);
void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir);
void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir, float radius, const Vector3 &ringNormal, int numAsteroids = 20);

void emplaceWeaponMachineGun(GameContext &context, entt::entity entity);
void emplaceWeaponBasic(GameContext &context, entt::entity entity);
void emplaceWeaponSniper(GameContext &context, entt::entity entity);
void emplaceWeaponBurstSniper(GameContext &context, entt::entity entity);
void emplaceWeaponShotgun(GameContext &context, entt::entity entity);
void emplaceWeaponBigBall(GameContext &context, entt::entity entity);

void emplaceRandomWeapon(GameContext &context, entt::entity turret);
void emplaceRandomWeapon(GameContext &context, entt::entity turret, int value);
#endif // ENTITIES_HPP