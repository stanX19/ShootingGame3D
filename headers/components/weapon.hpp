#pragma once

#include "includes.hpp"
#include <string>

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
	float value;
};

struct AmmoReload
{
	float cd;
	float timer;

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
	float totalChargeNeeded;
	Color effectColor = WHITE;
	float chargeAmmo = 1.0f;
	float currentCharge = 0.0f;
	entt::entity chargeEffectEntity = entt::null;

	ChargedWeapon(float chargeTime, Color color = WHITE)
		: totalChargeNeeded(chargeTime), effectColor(color) {}
};

struct WeaponParent
{
	entt::entity parent;
};

namespace tag {
	namespace weapon {
		struct IsWeapon {};
		struct ParentControlledAim {};
		struct FollowParentAim {};
		struct AIControlledAim {};
		struct PlayerControlledFire {};
		struct ParentControlledFire {};
		struct AIControlledFire {};
		struct FollowParentFire {};
		struct IsSpecialWeapon {};

		struct FireRequest {};
		struct IsFiring {};
		struct CanFire {};
	}
	namespace bullet_type {
		struct Kinetic {};
		struct Energy {};
		struct Lazer {};
	}
}
