#include "weapons.hpp"
#include "utils.hpp"
#include "constants.hpp"

namespace {
	const float LAZER_SPEED = 100000.0f;
	const float BASE_DAMAGE = 10.0f;
	const Vector3 LAZER_BOUND = {ARENA_SIZE * 2, ARENA_SIZE * 2, ARENA_SIZE * 2};
	
	// effective range and angle is linearly inverse, just multiply the angles to get different distances
	const float EFFECTIVE_RANGE = COMBAT_DIST;
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
		context.templateReg.emplace<DisappearBound>(bullet, LAZER_BOUND * -1, LAZER_BOUND);
		return bullet;
	}
}

void weapon::emplaceWeaponLazerBasic(GameContext &context, entt::entity entity)
{
	const float rad = 0.05f;
	
	entt::entity bulletTemplate = createBulletTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 1.0f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = LAZER_SPEED;

	emplaceLazerWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.20});
}

void weapon::emplaceWeaponLazerMachineGun(GameContext &context, entt::entity entity)
{
	const float rad = 0.05f;
	
	entt::entity bulletTemplate = createBulletTemplate(context, rad, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{1.0f});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{BASE_DAMAGE * 0.5f});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(BASE_SPREAD * 5);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = LAZER_SPEED;

	emplaceLazerWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{30.0f, 60});
	context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{6.0});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{0.05});
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

	emplaceLazerWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{2.0f, 5});
	context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{0.25});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{1.0f});
}
