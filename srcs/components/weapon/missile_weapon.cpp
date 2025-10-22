#include "weapons.hpp"
#include "utils.hpp"
#include "constants.hpp"

namespace {
	const float MISSILE_SPEED = 100.0f;
	const float MISSILE_MAX_SPEED = 400.0f;
	const float MISSILE_LIFESPAN = 20.0f;
	const float BASE_DAMAGE = 250.0f;
	const Vector3 MISSILE_BOUND = {ARENA_SIZE * 2, ARENA_SIZE * 2, ARENA_SIZE * 2};

	// effective range and angle is linearly inverse, just multiply the angles to get different distances
	const float EFFECTIVE_RANGE = COMBAT_DIST * 5;
	const float BASE_SPREAD = std::atan2(1.0f, EFFECTIVE_RANGE);  // atan(ENEMY_RAD, EFFECTIVE_RANGE)

	const Color BASE_COLOR = GRAY;

	Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity, Color baseColor = BASE_COLOR)
	{
		// if (context.registry.any_of<RenderBody>(entity))
		// 	return colorLerp(colorRevert(context.registry.get<RenderBody>(entity).color), baseColor, 0.4);
		return baseColor;
	}

	void emplaceMissileWeaponCommon(GameContext &context, entt::entity entity) {
		context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
		context.registry.emplace_or_replace<AimTarget>(entity);
		context.registry.emplace_or_replace<AimDirection>(entity);
	}

	entt::entity createMissileTemplate(GameContext &context, float rad, Color color) {
		entt::entity missile = context.templateReg.create();
		t_model_id model = context.modelManager.loadModel("assets/Models/missile/missile.glb");
		// context.templateReg.emplace<tag::Targetable>(missile);
		context.templateReg.emplace<tag::VelocitySyncModelRot>(missile);
		context.templateReg.emplace<CollisionBody>(missile, CollisionBody{rad});
		context.templateReg.emplace<RenderBody>(missile, RenderBody{model, color, rad});
		context.templateReg.emplace<DisappearBound>(missile, MISSILE_BOUND * -1, MISSILE_BOUND);
		context.templateReg.emplace<Lifespan>(missile, Lifespan{MISSILE_LIFESPAN});
		context.templateReg.emplace<SpawnsTrailParticle>(missile, SpawnsTrailParticle{rad * 0.5f, 0.5f});
		context.templateReg.emplace<Rotation>(missile);
		context.templateReg.emplace<tag::VelocitySyncRot>(missile);
		context.templateReg.emplace<MoveTarget>(missile);
		return missile;
	}
}

void weapon::emplaceWeaponMissileBasic(GameContext &context, entt::entity entity)
{
	const float rad = 0.5f;

	entt::entity bulletTemplate = createMissileTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE});
	context.templateReg.emplace<tag::effect::ExplodeOnDeath>(bulletTemplate);
	context.templateReg.emplace<ScalarAcceleration>(bulletTemplate, ScalarAcceleration{100.0f});
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{0.5f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = MISSILE_SPEED;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{2.0f, 2.0f});
	context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{1.0f / 15.0f});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{1.0f});
}

void weapon::emplaceWeaponMissileSwarm(GameContext &context, entt::entity entity)
{
	const float rad = 0.15f;

	entt::entity bulletTemplate = createMissileTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<tag::effect::ExplodeOnDeath>(bulletTemplate);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{0.75f});
	// context.templateReg.emplace<DelayedDamage>(bulletTemplate, DelayedDamage{MISSILE_LIFESPAN * 0.2f, 1000000.0f});
	context.templateReg.emplace_or_replace<Lifespan>(bulletTemplate, Lifespan{MISSILE_LIFESPAN * 0.2f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(PI / 4);
	weapon.bulletData.bulletCount = 4;
	weapon.bulletData.speed = MISSILE_SPEED * 2.0f;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{3.0f, 3.0f});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{8.0f});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.25f});
}

void weapon::emplaceWeaponMissileNuke(GameContext &context, entt::entity entity)
{
	const float rad = 2.0f;

	entt::entity bulletTemplate = createMissileTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{250.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 10.0f});
	context.templateReg.emplace<tag::effect::ExplodeOnDeath>(bulletTemplate);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{1.5f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = MISSILE_SPEED * 0.5f;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{1.0f, 1.0f});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{15.0f});
}

void weapon::emplaceWeaponMissileFlares(GameContext &context, entt::entity entity)
{
	const float rad = 0.15f;

	entt::entity bulletTemplate = createMissileTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<tag::Targetable>(bulletTemplate);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{0.05f});
	// context.templateReg.emplace<DelayedDamage>(bulletTemplate, DelayedDamage{MISSILE_LIFESPAN * 0.2f, 1000000.0f});
	context.templateReg.emplace_or_replace<Lifespan>(bulletTemplate, Lifespan{MISSILE_LIFESPAN * 0.2f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(PI * 0.5f);
	weapon.bulletData.bulletCount = 2;
	weapon.bulletData.speed = MISSILE_SPEED * 0.5f;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{6.0f, 6.0f});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{8.0f});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.125f});
}