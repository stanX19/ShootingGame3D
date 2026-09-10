#ifndef COMPONENTS_WEAPON_HPP
#define COMPONENTS_WEAPON_HPP

#include "includes.hpp"
#include <string>

namespace weapon {

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

struct Bullet {};
struct Missile {};

struct Kinetic {};
struct Energy {};
struct Lazer {};

namespace weapon = ::weapon::tag;
namespace bullet_type = ::weapon::tag;

} // namespace tag

} // namespace weapon

namespace tag {
using ::weapon::tag::IsWeapon;
using ::weapon::tag::ParentControlledAim;
using ::weapon::tag::FollowParentAim;
using ::weapon::tag::AIControlledAim;
using ::weapon::tag::PlayerControlledFire;
using ::weapon::tag::ParentControlledFire;
using ::weapon::tag::AIControlledFire;
using ::weapon::tag::FollowParentFire;
using ::weapon::tag::IsSpecialWeapon;
using ::weapon::tag::FireRequest;
using ::weapon::tag::IsFiring;
using ::weapon::tag::CanFire;
using ::weapon::tag::Bullet;
using ::weapon::tag::Missile;
using ::weapon::tag::Kinetic;
using ::weapon::tag::Energy;
using ::weapon::tag::Lazer;

namespace weapon = ::weapon::tag;
namespace bullet_type = ::weapon::tag;
} // namespace tag

using ::weapon::AimTarget;
using ::weapon::AimDirection;
using ::weapon::WeaponName;
using ::weapon::Weapon;
using ::weapon::WeaponCooldown;
using ::weapon::Ammo;
using ::weapon::AmmoRegen;
using ::weapon::AmmoReload;
using ::weapon::JustFired;
using ::weapon::ExtendFireDuration;
using ::weapon::ExtendFireRequest;
using ::weapon::ChargedWeapon;
using ::weapon::WeaponParent;

#endif // COMPONENTS_WEAPON_HPP
