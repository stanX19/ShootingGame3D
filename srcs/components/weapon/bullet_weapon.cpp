#include "weapons.hpp"
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

	void emplaceBulletWeaponCommon(GameContext &context, entt::entity entity) {
		context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
		context.registry.emplace_or_replace<AimTarget>(entity);
		context.registry.emplace_or_replace<AimDirection>(entity);
	}

	entt::entity createBulletTemplate(GameContext &context) {
		entt::entity bullet = context.templateReg.create();
		context.templateReg.emplace<tag::Bullet>(bullet);
		context.templateReg.emplace<tag::VelocitySyncModelRot>(bullet);
		context.templateReg.emplace<tag::bullet_type::Kinetic>(bullet);
		context.templateReg.emplace<ModelStrech>(bullet, 1.0f);
		return bullet;
	}
}

void weapon::emplaceWeaponMachineGun(GameContext &context, entt::entity entity)
{
	const float radius = 0.05f;
	t_model_id model = context.meshManager.createSphere();
	
	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 0.5f});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{10.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 5);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 2.0f;

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{20.0, 30});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{2.5});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.1});
	emplaceBulletWeaponCommon(context, entity);
}

void weapon::emplaceWeaponShotgun(GameContext &context, entt::entity entity)
{
	const float radius = 0.1f;
	t_model_id model = context.meshManager.createSphere();

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 0.75f});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{5.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 50.0f);
	weapon.bulletData.bulletCount = 10;
	weapon.bulletData.speed = BASE_SPEED * 2.0f;

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0.0, 3});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{0.5});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.5});
	emplaceBulletWeaponCommon(context, entity);
}

void weapon::emplaceWeaponBigBall(GameContext &context, entt::entity entity)
{
	const float radius = 0.5f;
	t_model_id model = context.meshManager.createSphere();

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 5});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{15.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = 0.0f;
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 1.5f;

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{2.5});
	emplaceBulletWeaponCommon(context, entity);
}

void weapon::emplaceWeaponSniper(GameContext &context, entt::entity entity)
{
	const float radius = 0.2f;
	t_model_id model = context.meshManager.createSphere();

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{50.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 2.5f});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{10.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 1.0f);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 3.0f;

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.75});
	emplaceBulletWeaponCommon(context, entity);
}


void weapon::emplaceWeaponBurstSniper(GameContext &context, entt::entity entity)
{
	// Constants
	const float radius = 0.2f;
	t_model_id model = context.meshManager.createSphere();

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{50.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 2.5f});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{10.0f});

	// Shooting Configurations
	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 2);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 3.0f;

	// Weapon Components
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.1});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0, 5});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{0.75});
	emplaceBulletWeaponCommon(context, entity);
}

void weapon::emplaceWeaponBasic(GameContext &context, entt::entity entity)
{
	const float radius = 0.075f;
	t_model_id model = context.meshManager.createSphere();

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{10.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 5);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 1.2f;

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.25});
	emplaceBulletWeaponCommon(context, entity);
}


