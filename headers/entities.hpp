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
entt::entity spawnPlayer(GameContext &context);
entt::entity spawnTurret(GameContext &context, Color color);
void spawnBullet(GameContext &context, Position pos, Velocity velocity, HP hp,
				 Damage damage, float rad, Color color, Lifetime lifetime);
void spawnDebris(GameContext &context, const Vector3 &position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad);

void emplaceWeaponMachineGun(GameContext &context, entt::entity entity);
void emplaceWeaponBasic(GameContext &context, entt::entity entity);
void emplaceWeaponSniper(GameContext &context, entt::entity entity);


#endif // ENTITIES_HPP