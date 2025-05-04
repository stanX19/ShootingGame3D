#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "includes.hpp"

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

struct Weapon {
	float cd;
	float damage;
	float radius;
};

struct Bullet {}; // Tag component for bullets

#endif