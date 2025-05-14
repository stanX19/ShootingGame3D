#include "shoot_3d.hpp"

static Color getColor(entt::registry &registry, entt::entity entity)
{
	if (registry.any_of<RenderBody>(entity))
		return colorMix(colorRevert(registry.get<RenderBody>(entity).color), WHITE, 1.0f, 0.5f);
	return WHITE;
}

void emplaceWeaponMachineGun(entt::registry &registry, entt::entity entity)
{
	BulletWeapon weapon;
	weapon.firing = false;
	weapon.shootCooldown = 0.1f;
	weapon.timeSinceLastShot = 0.0f;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = 1000.0f;
	weapon.bulletData.speed = 100.0f;
	weapon.bulletData.rad = 0.05f;
	weapon.bulletData.color = getColor(registry, entity);
	weapon.bulletData.lifetime = 10.0f;

	Ammo ammo;
	ammo.value = 5.0f;
	ammo.maxValue = 15;

	AmmoReload reload;
	reload.value = 3.0f; // reload per second

	registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	registry.emplace_or_replace<Ammo>(entity, ammo);
	registry.emplace_or_replace<AmmoReload>(entity, reload);
}

void emplaceWeaponSniper(entt::registry &registry, entt::entity entity)
{
	BulletWeapon weapon;
	weapon.firing = false;
	weapon.shootCooldown = 2.0f;
	weapon.timeSinceLastShot = 0.0f;

	weapon.bulletData.hp = 50.0f;
	weapon.bulletData.dmg = 5000.0f;
	weapon.bulletData.speed = 200.0f;
	weapon.bulletData.rad = 0.2f;
	weapon.bulletData.color = getColor(registry, entity);
	weapon.bulletData.lifetime = 10.0f;

	registry.emplace_or_replace<BulletWeapon>(entity, weapon);
}

void emplaceWeaponBasic(entt::registry &registry, entt::entity entity)
{
	BulletWeapon weapon;
	weapon.firing = false;
	weapon.shootCooldown = 0.5f;
	weapon.timeSinceLastShot = 0.0f;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = 1000.0f;
	weapon.bulletData.speed = 60.0f;
	weapon.bulletData.rad = 0.075f;
	weapon.bulletData.color = getColor(registry, entity);
	weapon.bulletData.lifetime = 10.0f;

	registry.emplace_or_replace<BulletWeapon>(entity, weapon);
}