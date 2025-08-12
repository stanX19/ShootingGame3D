#include "entities.hpp"
#include "utils.hpp"

namespace
{
	const float BASE_SPEED = 400.0f;
	const float BASE_DAMAGE = 15.0f;
	const float EFFECTIVE_RANGE = 2000.0f;
	// effective range and angle is linearly inverse, just multiply to get different distances
	// atan(ENEMY_RAD, EFFECTIVE_RANGE)
	const float BASE_SPREAD = std::atan2(1.0f, EFFECTIVE_RANGE);

	Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity, Color baseColor = WHITE)
	{
		// if (context.registry.any_of<RenderBody>(entity))
		// 	return colorLerp(colorRevert(context.registry.get<RenderBody>(entity).color), baseColor, 0.4);
		return baseColor;
	}
}

void emplaceWeaponMachineGun(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 0.5f;
	weapon.bulletData.speed = BASE_SPEED * 2.0f;
	weapon.bulletData.rad = 0.05f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 5);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{20.0, 30});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{2.5});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.1});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Bullet>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponShotgun(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 0.5f;
	weapon.bulletData.speed = BASE_SPEED * 2.0f;
	weapon.bulletData.rad = 0.1f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 5.0f;
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 50.0f);
	weapon.bulletData.bulletCount = 10;

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0.0, 5});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{1.0});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.5});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Bullet>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponBigBall(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 10;
	weapon.bulletData.speed = BASE_SPEED * 1.5f;
	weapon.bulletData.rad = 1.0f;
	weapon.bulletData.color = getColor(context, entity);
	;
	weapon.bulletData.lifetime = 15.0f;

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{2.5});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Bullet>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponSniper(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 50.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 2.5f;
	weapon.bulletData.speed = BASE_SPEED * 3.0f;
	weapon.bulletData.rad = 0.2f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 1.0f);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.75});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Bullet>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponBurstSniper(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 50.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 2.5f;
	weapon.bulletData.speed = BASE_SPEED * 3.0f;
	weapon.bulletData.rad = 0.2f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 2);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.1});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0, 5});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{0.75});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Bullet>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponBasic(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 1.0f;
	weapon.bulletData.speed = BASE_SPEED * 1.2f;
	weapon.bulletData.rad = 0.075f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 5);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.25});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Bullet>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}