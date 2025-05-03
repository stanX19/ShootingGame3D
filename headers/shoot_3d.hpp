#ifndef SHOOT_3D_HPP
#define SHOOT_3D_HPP
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raylib.h"
#include "raymath.h"
#pragma GCC diagnostic pop
#include <entt.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <iostream>

const float BULLET_SPEED = 40.0f;
const float PLAYER_SPEED = 25.0f;
const float SHOOT_COOLDOWN = 0.15f;
const float BULLET_LIFETIME = 10.0f;
const float ENEMY_SPEED = 5.0f;
const float ENEMY_ATTACK_COOLDOWN = 1.5f;
const int ENEMY_COUNT = 10;
const float ARENA_SIZE = 200.0f;
const float MOUSE_SENSITIVITY = 0.1f; // Reduced sensitivity, now used for keyboard
const int BULLET_DAMAGE = 10;
const int ENEMY_BULLET_DAMAGE = 10;

// Components
struct Position {
    Vector3 value;
};

struct Velocity {
    Vector3 value;
};

struct Rotation {
    Vector3 value; // pitch, yaw, roll in radians
};

struct Body {
    float radius;
    Color color;
};

struct HP {
    int value;
    int maxValue;
};

struct Damage {
    int value;
};

struct Player {
    float moveSpeed;
    float shootCooldown;
    float timeSinceLastShot;
};

struct Enemy {
    float moveSpeed;
    float attackCooldown;
    float timeSinceLastAttack;
};

struct Lifetime {
    float value;
};

struct Bullet {}; // Tag component for bullets

// Game constants
extern const float BULLET_SPEED;
extern const float PLAYER_SPEED;
extern const float SHOOT_COOLDOWN;
extern const float BULLET_LIFETIME;
extern const float ENEMY_SPEED;
extern const float ENEMY_ATTACK_COOLDOWN;
extern const int ENEMY_COUNT;
extern const float ARENA_SIZE;
extern const float MOUSE_SENSITIVITY; // Reduced sensitivity, now used for keyboard
extern const int BULLET_DAMAGE;
extern const int ENEMY_BULLET_DAMAGE;

// Utility functions
Vector3 GetForwardVector(const Rotation& rotation);
Vector3 GetRightVector(const Rotation& rotation);
Vector3 GetUpVector();

// Game systems
void spawn_bullet(entt::registry& registry, const Vector3& pos, const Vector3& dir, int damage, Color color);
void spawn_enemy(entt::registry& registry, const Vector3& pos);
void spawn_debris(entt::registry& registry, const Vector3& position, float originalRadius, Color originalColor, int count = 8, float lifespan = 2.0f);

namespace ecs_system {
	void player_update(entt::registry& registry, float dt);
	void enemy_update(entt::registry& registry, float dt);
	void entity_movement(entt::registry& registry, float dt);
	void entity_collision(entt::registry& registry);
	void entity_lifetime(entt::registry& registry, float dt);
	void entity_cleanup(entt::registry& registry);
	void enemy_respawn(entt::registry& registry);
}

#endif // SHOOT_3D_HPP