#include "entities.hpp"
#include "utils.hpp"

static Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity)
{
	// if (context.registry.any_of<RenderBody>(entity))
	// 	return colorMix(colorRevert(context.registry.get<RenderBody>(entity).color), WHITE, 1.0f, 0.5f);
	return WHITE;
}

void emplaceWeaponMachineGun(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = 1000.0f;
	weapon.bulletData.speed = 400.0f;
	weapon.bulletData.rad = 0.05f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(1.00 * DEG2RAD);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{5.0, 30});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{3.0});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.1});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
    context.registry.emplace_or_replace<AimTarget>(entity);
    context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponShotgun(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = 500.0f;
	weapon.bulletData.speed = 200.0f;
	weapon.bulletData.rad = 0.025f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(10 * DEG2RAD);
	weapon.bulletData.bulletCount = 20;

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0.0, 3});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{2.0});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.05});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
    context.registry.emplace_or_replace<AimTarget>(entity);
    context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponBigBall(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = 15000.0f;
	weapon.bulletData.speed = 300.0f;
	weapon.bulletData.rad = 3.0f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 15.0f;

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{7.0});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
    context.registry.emplace_or_replace<AimTarget>(entity);
    context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponSniper(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 50.0f;
	weapon.bulletData.dmg = 2500.0f;
	weapon.bulletData.speed = 600.0f;
	weapon.bulletData.rad = 0.2f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(0.10 * DEG2RAD);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.75});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
    context.registry.emplace_or_replace<AimTarget>(entity);
    context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponBurstSniper(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 50.0f;
	weapon.bulletData.dmg = 2500.0f;
	weapon.bulletData.speed = 600.0f;
	weapon.bulletData.rad = 0.2f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(0.20 * DEG2RAD);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.1});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0, 5});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{1.00});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
    context.registry.emplace_or_replace<AimTarget>(entity);
    context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponBasic(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = 1000.0f;
	weapon.bulletData.speed = 240.0f;
	weapon.bulletData.rad = 0.075f;
	weapon.bulletData.color = getColor(context, entity);
	weapon.bulletData.lifetime = 10.0f;
	weapon.bulletData.spreadSin = std::sin(0.50 * DEG2RAD);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.25});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
    context.registry.emplace_or_replace<AimTarget>(entity);
    context.registry.emplace_or_replace<AimDirection>(entity);
}