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
    float value;
    float maxValue;
};

struct HPRegen {
    float value;  // health regen per second
};

struct Damage {
    float value;
};

struct Player {
    float moveSpeed;
};

struct Enemy {
    float moveSpeed;
};

struct Lifetime {
    float value;
};


// weapon components
struct BulletWeapon {
	struct {
		float hp;
		float dmg;
		float speed;
		float rad;
		Color color;
		float lifetime;
	} bulletData;
	bool firing;
    float shootCooldown;
    float timeSinceLastShot;
};

struct AimRotation {
    Vector3 value; // pitch, yaw, roll in radians
};

struct Ammo {
	float value;
	float maxValue;
};

struct AmmoReload {
	float value;
};
// end of weapon components

struct Bullet {}; // Tag component for bullets

#endif