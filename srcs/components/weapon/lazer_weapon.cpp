#include "weapons.hpp"
#include "utils.hpp"
#include "constants.hpp"

namespace {
	const float LAZER_SPEED = 10000.0f;
	const float BASE_DAMAGE = 10.0f;
	const Vector3 LAZER_BOUND = {ARENA_SIZE, ARENA_SIZE, ARENA_SIZE};
	
	// effective range and angle is linearly inverse, just multiply the angles to get different distances
	const float EFFECTIVE_RANGE = 1000.0f;
	const float BASE_SPREAD = std::atan2(1.0f, EFFECTIVE_RANGE);  // atan(ENEMY_RAD, EFFECTIVE_RANGE)

	const Color BASE_COLOR = GREEN;

	Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity, Color baseColor = BASE_COLOR)
	{
		// if (context.registry.any_of<RenderBody>(entity))
		// 	return colorLerp(colorRevert(context.registry.get<RenderBody>(entity).color), baseColor, 0.4);
		return baseColor;
	}

	void emplaceLazerWeaponCommon(GameContext &context, entt::entity entity) {
		context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
		context.registry.emplace_or_replace<AimTarget>(entity);
		context.registry.emplace_or_replace<AimDirection>(entity);
		context.registry.emplace<DisappearBound>(entity, LAZER_BOUND * -2, LAZER_BOUND * 2);
	}

	entt::entity createBulletTemplate(GameContext &context, float rad, Color color) {
		entt::entity bullet = context.templateReg.create();
		t_model_id model = context.modelManager.createCube();
		context.templateReg.emplace<tag::Bullet>(bullet);
		context.templateReg.emplace<tag::VelocitySyncModelRot>(bullet);
		context.templateReg.emplace<tag::bullet_type::Energy>(bullet);
		context.templateReg.emplace<ModelStrech>(bullet, 1.0f / (rad * 2));  // 1 / diameter
		context.templateReg.emplace<CollisionBody>(bullet, CollisionBody{rad});
		context.templateReg.emplace<RenderBody>(bullet, RenderBody{model, color, rad});
		return bullet;
	}
}

void weapon::emplaceWeaponLazerBasic(GameContext &context, entt::entity entity)
{
	const float rad = 0.1f;
	
	entt::entity bulletTemplate = createBulletTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 1.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = LAZER_SPEED;

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.40});
	emplaceLazerWeaponCommon(context, entity);
}

void weapon::emplaceWeaponLazerMachineGun(GameContext &context, entt::entity entity)
{
	const float rad = 0.1f;
	
	entt::entity bulletTemplate = createBulletTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 0.4f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = LAZER_SPEED;

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{30.0f, 60});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{6.0});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.05});
	emplaceLazerWeaponCommon(context, entity);
}

void weapon::emplaceWeaponLazerDeletor(GameContext &context, entt::entity entity)
{
	const float rad = 1.0f;
	
	entt::entity bulletTemplate = createBulletTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 10});
	
	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = 0.0f;
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = LAZER_SPEED;

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0.0f, 3});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{0.25});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{1.0f});
	emplaceLazerWeaponCommon(context, entity);
}
