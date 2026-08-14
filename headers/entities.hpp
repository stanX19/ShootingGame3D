#ifndef ENTITIES_HPP
#define ENTITIES_HPP

#include "includes.hpp"
#include "components.hpp"
#include "game_context.hpp"
#include "entities/spaceship_factory.hpp"
#include "entities/unit.hpp"
#include "entities/turret.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

entt::entity spawnUnit(GameContext& context, const Vector3& pos, faction::Faction faction);
entt::entity spawnFastEliteUnit(GameContext& context, const Vector3& pos, faction::Faction faction);
entt::entity spawnEliteUnit(GameContext& context, const Vector3& pos, faction::Faction faction);
entt::entity spawnTerminatorUnit(GameContext& context, const Vector3& pos, faction::Faction faction);
entt::entity spawnMothershipUnit(GameContext& context, const Vector3& pos, faction::Faction faction);
entt::entity spawnPlayer(GameContext& context);
entt::entity spawnPlayer(GameContext& context, Vector3 pos);

void spawnDebris(
	GameContext& context,
	const Vector3& position,
	float originalRadius,
	Color originalColor,
	int count = 8,
	float lifespan = 2.0f,
	Vector3 velocity = {0, 0, 0}
);
void spawnDebris(
	GameContext& context,
	const Vector3& position,
	const RenderBody* bodyPtr,
	float lifespan = 2.0f,
	Vector3 velocity = {0, 0, 0}
);
void spawnExplosion(
	GameContext& context,
	const Vector3& pos,
	float rad,
	Vector3 velocity = {0, 0, 0},
	entt::entity parent = entt::null
);
void spawnExplosion(
	GameContext& context,
	const Vector3& pos,
	float rad,
	Vector3 velocity = {0, 0, 0},
	float lifespan = 5.0f,
	Color color = effect::EXPLOSION_COLOR,
	entt::entity parent = entt::null
);
void spawnExplosion(
	GameContext& context,
	const Vector3& pos,
	const effect::ExplodeOnDeath& effect,
	Vector3 velocity = {0, 0, 0},
	entt::entity parent = entt::null
);
void spawnInstantDamage(
	GameContext& context,
	const Vector3& pos,
	const effect::InstantDamageOnDeath& effect,
	entt::entity parent = entt::null
);
void spawnAsteroid(GameContext& context, const Vector3& pos, const Vector3& dir);
void spawnAsteroid(GameContext& context, const Vector3& pos, const Vector3& dir, float rad);
void spawnRingAsteroid(GameContext& context, const Vector3& center, const Vector3& dir);
void spawnRingAsteroid(
	GameContext& context,
	const Vector3& center,
	const Vector3& dir,
	float radius,
	const Vector3& ringNormal,
	int numAsteroids = 20
);
void spawnSunAndStars(GameContext& context, int numStars = 100);

#endif // ENTITIES_HPP
