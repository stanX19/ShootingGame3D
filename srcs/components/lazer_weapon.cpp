#include "entities.hpp"
#include "utils.hpp"

namespace {
	const float LAZER_SPEED = 10000.0f;
	const float LAZER_LIFETIME = 1.0f;
	const float BASE_DAMAGE = 5.0f;
	const float EFFECTIVE_RANGE = 500.0f;
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

void emplaceWeaponLazerBasic(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 1.0f;
	weapon.bulletData.speed = LAZER_SPEED;
	weapon.bulletData.rad = 0.1f;
	weapon.bulletData.color = getColor(context, entity, GREEN);
	weapon.bulletData.lifetime = LAZER_LIFETIME;
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.20});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Lazer>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponLazerMachineGun(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 0.4f;
	weapon.bulletData.speed = LAZER_SPEED;
	weapon.bulletData.rad = 0.1f;
	weapon.bulletData.color = getColor(context, entity, GREEN);
	weapon.bulletData.lifetime = LAZER_LIFETIME;
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{30.0f, 60});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{5.0});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.05});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Lazer>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}

void emplaceWeaponLazerDeletor(GameContext &context, entt::entity entity)
{
	BulletWeapon weapon;

	weapon.bulletData.hp = 1.0f;
	weapon.bulletData.dmg = BASE_DAMAGE * 10;
	weapon.bulletData.speed = LAZER_SPEED;
	weapon.bulletData.rad = 1.0f;
	weapon.bulletData.color = getColor(context, entity, GREEN);
	weapon.bulletData.lifetime = LAZER_LIFETIME;
	weapon.bulletData.spreadSin = 0.0f; // std::sin(BASE_SPREAD * 10);

	context.registry.emplace_or_replace<BulletWeapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0.0f, 3});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{0.25});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{1.0f});
	context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
	context.registry.emplace_or_replace<tag::weapon::type::Lazer>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	context.registry.emplace_or_replace<AimDirection>(entity);
}