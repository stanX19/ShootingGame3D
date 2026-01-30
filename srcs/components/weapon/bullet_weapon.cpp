#include "weapons.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "factions.hpp"
#include "components/sound.hpp"

namespace
{
	const float BASE_SPEED = 800.0f;
	const float BASE_DAMAGE = 25.0f;
	const float EFFECTIVE_RANGE = COMBAT_DIST * 2;
	const Vector3 BULLET_BOUND = {ARENA_SIZE + COMBAT_DIST * 2, ARENA_SIZE + COMBAT_DIST * 2, ARENA_SIZE + COMBAT_DIST * 2};
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
		context.registry.emplace_or_replace<sound::ShootSound>(entity, sound::RANDOM_BULLET_SHOOT, 0.5f);
	}

	entt::entity createBulletTemplate(GameContext &context) {
		entt::entity bullet = context.templateReg.create();
		context.templateReg.emplace<tag::Bullet>(bullet);
		context.templateReg.emplace<faction::Faction>(bullet, faction::FAC_BULLET);
		context.templateReg.emplace<tag::VelocitySyncModelRot>(bullet);
		context.templateReg.emplace<tag::bullet_type::Kinetic>(bullet);
		context.templateReg.emplace<ModelStrech>(bullet, 1.0f);
		context.templateReg.emplace<DisappearBound>(bullet, BULLET_BOUND * -1, BULLET_BOUND);
		context.templateReg.emplace<sound::HitSound>(bullet, sound::RANDOM_BULLET_HIT, 0.4f);
		return bullet;
	}
}

void weapon::emplaceWeaponMachineGun(GameContext &context, entt::entity entity)
{
	const float radius = 0.05f;
	t_model_id model = context.modelManager.createSphere();
	
	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 1.00f});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{10.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 10);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 2.0f;

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{80.0, 80});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{5.5f});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.1});
}

void weapon::emplaceWeaponShotgun(GameContext &context, entt::entity entity)
{
	const float radius = 0.025f;
	t_model_id model = context.modelManager.createSphere();

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 1.00f});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{5.0f});
	context.templateReg.emplace_or_replace<ModelStrech>(bulletTemplate, ModelStrech{0.5f / (radius * 2)});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 100.0f);
	weapon.bulletData.bulletCount = 10;
	weapon.bulletData.speed = BASE_SPEED * 2.0f;

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{5.0f, 5.0f});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{1.5f});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.1});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{1.0f});
}

void weapon::emplaceWeaponBigBall(GameContext &context, entt::entity entity)
{
	const float radius = 1.5f;
	t_model_id model = context.modelManager.loadModel("assets/Models/asteroid/asteroid_ceres.glb", Vector3{0.36f, 0.36f, 0.38f}, Vector3UnitZ, Vector3{0.5f, 0.75f, 0.5f});;

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{1000.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 15.0});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{15.0f});
	context.templateReg.emplace<Rotation>(bulletTemplate);
	context.templateReg.emplace<RotationVelocity>(bulletTemplate, QuaternionFromAxisAngle(Vector3UnitY, PI));
	// context.templateReg.emplace<RadiusExpand>(bulletTemplate, 1.0f);
	context.templateReg.erase<ModelStrech>(bulletTemplate);

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = 0.0f;
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 0.25f;

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0.0f, 1.0f});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{7.0f});
}

void weapon::emplaceWeaponSniper(GameContext &context, entt::entity entity)
{
	const float radius = 0.2f;
	t_model_id model = context.modelManager.createSphere();

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{50.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 2.5f});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{10.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 0.01f);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 6.0f;

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{1.5f});
}


void weapon::emplaceWeaponBurstSniper(GameContext &context, entt::entity entity)
{
	// Constants
	const float radius = 0.2f;
	t_model_id model = context.modelManager.createSphere();

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{50.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 2.5f});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{10.0f});

	// Shooting Configurations
	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 5.0f);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = BASE_SPEED * 6.0f;

	// Weapon Components
	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.35});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{20, 20});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{12.0f});
}

void weapon::emplaceWeaponBasic(GameContext &context, entt::entity entity)
{
	const float radius = 0.075f;
	t_model_id model = context.modelManager.createSphere();

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

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.25});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{30, 30});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{2.0f});
}


