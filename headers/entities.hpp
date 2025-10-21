#ifndef ENTITIES_HPP
#define ENTITIES_HPP
#include "includes.hpp"
#include "components.hpp"
#include "game_context.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>

entt::entity spawnUnit(GameContext &context, const Vector3 &pos);
entt::entity spawnFastEliteUnit(GameContext &context, const Vector3 &pos);
entt::entity spawnEliteUnit(GameContext &context, const Vector3 &pos);
entt::entity spawnMothershipUnit(GameContext &context, const Vector3& pos);
entt::entity spawnPlayer(GameContext &context);
entt::entity spawnPlayer(GameContext &context, Vector3 pos);
entt::entity spawnUnlinkedAutoTurret(GameContext &context, Color color);
entt::entity spawnLinkedTurret(GameContext &context, Color color, entt::entity &parent, Vector3 relpos);
entt::entity spawnLinkedAutoTurret(GameContext &context, Color color, entt::entity &parent, Vector3 relpos);
void spawnDebris(GameContext &context, const Vector3 &position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f, Vector3 velocity = {0, 0, 0});
void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity = {0, 0, 0}, entt::entity parent = entt::null);
void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity = {0, 0, 0}, float lifespan = 5.0f, Color color = ORANGE, entt::entity parent = entt::null);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad);
void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir);
void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir, float radius, const Vector3 &ringNormal, int numAsteroids = 20);
void spawnSunAndStars(GameContext &context, int numStars=100);

#endif // ENTITIES_HPP