#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "includes.hpp"

// Components
struct Position
{
	Vector3 value;
};

struct Velocity
{
	Vector3 value = { 0, 0, 0 };
};

struct Rotation
{
	Quaternion value = QuaternionIdentity();
};

struct Body
{
	float radius;
	Color color;
};

struct HP
{
	float value;
	float maxValue;
};

struct HPRegen
{
	float value; // health regen per second
};

struct Damage
{
	float value;
};

struct Asteroid
{
};

struct Player
{
};

struct Enemy
{
};

struct EliteEnemy
{
};

struct MaxSpeed
{
	float value;
};

struct TurnSpeed
{
	float value;
};

struct Lifetime
{
	float value;
};

struct DisappearBound
{
	Vector3 start;
	Vector3 end;
};

// weapon components
struct BulletWeapon
{
	struct
	{
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

struct AimTarget
{
	entt::entity entity = entt::null;
};

struct AimDirection
{
	Vector3 value; // pitch, yaw, roll in radians
};

struct Ammo
{
	float value;
	float maxValue;
};

struct AmmoReload
{
	float value;
};
// end of weapon components

struct PlayerTargetable
{
	int distance = 3000;
	Vector3 toSelf = {0, 0, 0};
};

struct Bullet
{
}; // Tag component for bullets

#endif