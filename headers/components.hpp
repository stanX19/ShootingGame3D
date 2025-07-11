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
	Color color;
	Vector3 scale;
	Vector3 translation;
	Quaternion rotation;
	
	RenderBody(t_model_id id, float _scale)
		: modelID(id), color(WHITE), translation({0, 0, 0}), rotation({0, 0, 0, 0})
	{
		scale = Vector3{_scale, _scale, _scale};
	}
	
	RenderBody(t_model_id id, Color color, float _scale, Vector3 _translation = {0.0f, 0.0f, 0.0f}, Quaternion _rotation = {0.0f, 0.0f, 0.0f, 1.0f})
		: modelID(id), color(color), translation(_translation), rotation(_rotation)
	{
		scale = Vector3{_scale, _scale, _scale};
	}

	RenderBody(
		t_model_id id,
		Color _color = WHITE,
		Vector3 _scale = {1.0f, 1.0f, 1.0f},
		Vector3 _translation = {0.0f, 0.0f, 0.0f},
		Quaternion _rotation = {0.0f, 0.0f, 0.0f, 1.0f}
	)
		: modelID(id), color(_color), scale(_scale), translation(_translation), rotation(_rotation)
	{}
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
		int bulletCount = 1;
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
	float value;	// reload per second, the higher the faster
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

namespace tag {
	struct Asteroid {};
	struct Player {};
	struct Enemy {};
	struct EliteEnemy {};
	struct Bullet {};
	struct Targetable {};
	namespace weapon {
		struct IsWeapon {};
		struct ParentControlledAim {};		// dont do anything, parent updates AimTarget; TODO
		struct FollowParentAim {};			// follows parent's AimTarget
		struct AIControlledAim {};			// updates AimTarget automatically using rotation; TODO; need to do factions first
		struct PlayerControlledFire {};		// read keyboard or mouse inputs
		struct ParentControlledFire {};		// dont do anything, parent updates AimTarget; TODO
		struct AIControlledFire {};			// decide IsFiring using distance and angle; TODO
		struct FollowParentFire {};			// follow parent firing
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