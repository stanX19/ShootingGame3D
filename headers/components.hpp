#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "includes.hpp"
#include "model_manager.hpp"

// Components
struct Position
{
	Vector3 value = {0, 0, 0};
};

struct PrevPosition
{
	Vector3 value;
};

struct Velocity
{
	Vector3 value = { 0, 0, 0 };
};

struct Rotation
{
	Quaternion value = QuaternionUnitX;
};

struct CollisionBody
{
	float radius;
};

struct RenderBody
{
	t_model_id modelID;
    float scale = 1.0;
	Color color = WHITE;
    Vector3 translation = {0.0f, 0.0f, 0.0f};
    Quaternion rotation = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct PositionAnchor {
    entt::entity parent;
    Vector3 relpos;
};

struct RotationAnchor {
    entt::entity parent;
    Quaternion relrot = QuaternionUnitX;
};

struct DeathAnchor {
    entt::entity parent;
    float delay;
};

struct HP
{
	float value;
	float maxValue;

	HP(float val): value(val), maxValue(val) {}
};

struct HPRegen
{
	float value; // health regen per second
};

struct Damage
{
	float value;
};

struct MaxSpeed
{
	float value;
};

struct TurnSpeed
{
	float value;
};

struct Lifespan
{
	float value;
};

struct DelayedDamage
{
	float timeRemaining;
	float damage;
};

struct DisappearBound
{
	Vector3 start;
	Vector3 end;
};

// weapon components
struct AimTarget
{
	entt::entity entity = entt::null;
};

struct AimDirection
{
	Vector3 value = {0, 1, 0};
};

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
		float spreadSin = 0.0;
	} bulletData;
};

struct WeaponCooldown {
	float shootCooldown;
	float timeSinceLastShot = 0.0f;
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

struct JustFired
{
	float ammoCount;
};

struct WeaponParent
{
	entt::entity parent;
};
// end of weapon components

struct PlayerTargetable
{
	int distance = 3000;
	Vector3 toSelf = {0, 0, 0};
};

namespace tag {
	struct Asteroid {};
	struct Player {};
	struct Enemy {};
	struct EliteEnemy {};
	struct Bullet {};
	struct Targetable {};
	namespace weapon {
		struct IsWeapon {};
		struct ParentControlledAim {};		// dont do anything, parent updates AimTarget
		struct AIControlledAim {};			// updates AimTarget automatically using rotation; TODO; need to do factions first
		struct PlayerControlledShoot {};	// read keyboard or mouse inputs
		struct AIControlledShoot {};		// decide IsFiring using distance and angle; TODO
		struct ParentControlledShoot {};	// follow parent firing
		struct IsFiring {};
		struct CanFire {};
	};
	struct LightSource {};
	struct Shaded {};
	struct RotationSyncModel {};
	struct AimDirectionSyncModel {};
	struct GetVelOnAnchorDeath {};
}



#endif