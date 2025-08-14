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
				 Damage damage, float rad, Color color, Lifespan lifetime, ScoreParent scoreParent);
void spawnLazer(GameContext &context, Position pos, Velocity velocity, HP hp,
				 Damage damage, float rad, Color color, Lifespan lifetime, ScoreParent scoreParent);
void spawnDebris(GameContext &context, const Vector3 &position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f, Vector3 velocity = {0, 0, 0});
void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity = {0, 0, 0}, float lifespan = 5.0f, Color color = ORANGE);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir);
void spawnAsteroid(GameContext &context, const Vector3 &pos, const Vector3 &dir, float rad);
void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir);
void spawnRingAsteroid(GameContext &context, const Vector3 &center, const Vector3 &dir, float radius, const Vector3 &ringNormal, int numAsteroids = 20);
void spawnSunAndStars(GameContext &context, int numStars=100);

namespace weapon {
	void emplaceWeaponMachineGun(GameContext &context, entt::entity entity);
	void emplaceWeaponBasic(GameContext &context, entt::entity entity);
	void emplaceWeaponSniper(GameContext &context, entt::entity entity);
	void emplaceWeaponBurstSniper(GameContext &context, entt::entity entity);
	void emplaceWeaponShotgun(GameContext &context, entt::entity entity);
	void emplaceWeaponBigBall(GameContext &context, entt::entity entity);
	void emplaceWeaponLazerBasic(GameContext &context, entt::entity entity);
	void emplaceWeaponLazerMachineGun(GameContext &context, entt::entity entity);
	void emplaceWeaponLazerDeletor(GameContext &context, entt::entity entity);

	void emplaceRandomWeapon(GameContext &context, entt::entity turret);
	void emplaceRandomWeapon(GameContext &context, entt::entity turret, int value);
}

namespace weapon::utils {
	void assureBulletTypes(entt::registry &registry);
}

#endif // ENTITIES_HPP