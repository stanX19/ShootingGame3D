#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "includes.hpp"
#include "model_manager.hpp"
#include "collision_body_manager.hpp"

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

struct ScalarAcceleration
{
	float value = 0.0f;
};

struct Rotation
{
	Quaternion value = QuaternionUnitX;
};

struct PrevRotation
{
	Quaternion value = QuaternionUnitX;
};


struct RotationVelocity
{
	Quaternion value = QuaternionIdentity();  // rotation = rotation * (rotVel * dt)
};

struct TargetVelocity
{
	Vector3 value = { 0, 0, 0 };
	float lerpSpeed = 30.0f;
};

struct TargetRotation
{
	Quaternion value = QuaternionIdentity();
	float slerpSpeed = 30.0f;
};

struct CollisionBody
{
	float radius;
};

struct CollisionBodyModel
{
	t_collision_mesh_id modelID;
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

struct EnergyShield
{
	float hp;
	float maxHp;
	float activeDuration;
	float activeTimer = 0.0f;  // if > 0 will be rendered

	EnergyShield(float val, float _activeDuration=3.0f): hp(val), maxHp(val), activeDuration(_activeDuration) {}
};

struct EnergyShieldRegen
{
	float value;  // regen per second
	float regenCd = 2.0f;  // wait for deactivate 2 seconds
};

struct Damage
{
	float value;
};

struct Mass
{
	float value = 1.0f;
};

struct ImpulseRequest  // vel = vel + impulse  (directly, no dt)
{
	Vector3 value = {0, 0, 0};
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

// move components
struct MoveTarget
{
	entt::entity entity = entt::null;
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

struct WeaponName
{
	std::string value;
};

struct Weapon
{
    struct
    {
        float spreadSin = 0.0f;
        int bulletCount = 1;
        float speed = 400.0f;
    } bulletData;

    entt::entity bulletTemplate;

	Weapon(entt::entity templateEntity): bulletTemplate(templateEntity) {}
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

struct AmmoRegen
{
	float value;	// reload per second, the higher the faster
};

struct AmmoReload
{
	float cd;		// time needed to reload entire ammo
	float timer;	// cd --> 0

	AmmoReload(float _cd): cd(_cd), timer(cd) {}
};

struct JustFired
{
	float ammoCount;
};

struct ExtendFireDuration
{
	float duration = 0.0f;
	float timeRemaining = 0.0f;

	ExtendFireDuration(float t): duration(t), timeRemaining(0.0f) {}
};

struct ExtendFireRequest : ExtendFireDuration
{
	using ExtendFireDuration::ExtendFireDuration;
};

struct ChargedWeapon
{
	float totalChargeNeeded;		// time needed to fully charge
	Color effectColor = WHITE;
	float chargeAmmo = 1.0f;		// ammo consumption on full charge
	float currentCharge = 0.0f;		// current charge time
	entt::entity chargeEffectEntity = entt::null;

	ChargedWeapon(float chargeTime, Color color = WHITE)
		: totalChargeNeeded(chargeTime), effectColor(color) {}
};

struct WeaponParent
{
	entt::entity parent;
};
// end of weapon components

struct ScoreParent
{
	entt::entity parent = entt::null;
};

struct KilledScore
{
	int value;
};

struct Score {
	int value = 0;
};

struct ModelStrech {
	float scale;  // model scale front *= (scale * forward_speed)
};

struct RadiusExpand {
	float speed;  // radius += speed * dt
};

struct SpawnsTrailParticle {
	float radius = 0.5f;
	float lifespan = 1.0f;
	Color color = SKYBLUE;
};

struct Trail {
    Color color = WHITE;
    float rad = 0.1f;
    Trail(Color c = WHITE, float r = 0.1f): color(c), rad(r) {}
};

namespace tag {
	// todo: throw these into namespaces too
	struct Asteroid {};
	struct Missile {};
	struct AIMoveControl {};
	struct Suicidal {};
	struct Enemy {};
	struct EliteUnit {};
	struct Bullet {};
	struct Targetable {};
	struct Spaceship {};
	
	namespace effect {
		struct ExplodeOnDeath {};
		struct DropDebris {};
	}
	namespace weapon {
		struct IsWeapon {};
		struct ParentControlledAim {};		// dont do anything, parent updates AimTarget; TODO
		struct FollowParentAim {};			// follows parent's AimTarget
		struct AIControlledAim {};			// updates AimTarget automatically using rotation
		struct PlayerControlledFire {};		// read keyboard or mouse inputs
		struct ParentControlledFire {};		// dont do anything, parent updates AimTarget; TODO
		struct AIControlledFire {};			// decide IsFiring using distance and angle
		struct FollowParentFire {};			// follow parent firing
		struct IsSpecialWeapon {};			// special weapon, only fires on `E` key

		struct FireRequest {};
		struct IsFiring {};
		struct CanFire {};
	};
	namespace bullet_type {
		struct Kinetic {};
		struct Energy {};
		struct Lazer {};
	}
	struct LightSource {};
	struct Shaded {};
	struct SkyBox {};
	struct RotationSyncModel {};
	struct AimDirectionSyncModel {};
	struct VelocitySyncModelRot {};
	struct VelocitySyncRot {};
	struct GetVelOnAnchorDeath {};
}

#endif
