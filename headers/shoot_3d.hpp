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

// Utility functions
Vector3 GetForwardVector(const Rotation& rotation);
Vector3 GetRightVector(const Rotation& rotation);
Vector3 GetUpVector();
Vector3 GetUpVector(const Rotation& rotation);

// Game systems
void spawn_bullet(entt::registry& registry, const Vector3& pos, const Vector3& dir, float damage, Color color);
void spawn_enemy(entt::registry& registry, const Vector3& pos);
void spawn_debris(entt::registry& registry, const Vector3& position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f);
entt::entity spawn_player(entt::registry& registry);

namespace ecs_system {
	void player_update(entt::registry& registry, float dt);
	void enemy_update(entt::registry& registry, float dt);
	void entity_movement(entt::registry& registry, float dt);
	void entity_collision(entt::registry& registry);
	void entity_lifetime(entt::registry& registry, float dt);
	void hp_cleanup(entt::registry& registry);
	void enemy_respawn(entt::registry& registry);
	void ammo_reload(entt::registry& registry, float dt);
	void hp_regen(entt::registry& registry, float dt);
	void weapon_update(entt::registry& registry, float dt);
}

#endif // SHOOT_3D_HPP