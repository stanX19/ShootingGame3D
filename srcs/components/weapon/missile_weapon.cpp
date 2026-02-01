#include "weapons.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "components/sound.hpp"
#include "game_config.hpp"

namespace {
	const Color BASE_COLOR = GRAY;

	Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity, Color baseColor = BASE_COLOR)
	{
		return baseColor;
	}

	void emplaceMissileWeaponCommon(GameContext &context, entt::entity entity) {
		context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
		context.registry.emplace_or_replace<AimTarget>(entity);
		context.registry.emplace_or_replace<AimDirection>(entity);
		context.registry.emplace_or_replace<sound::ShootSound>(entity, sound::RANDOM_MISSILE_SHOOT, 0.5f);
	}

	entt::entity createMissileTemplate(GameContext &context, float rad, Color color) {
		const auto& cfg = context.config;
		float arenaSize = cfg.getFloat("game.arenaSize", 2000.0f);
		float lifespan = cfg.getFloat("weapons.missile.lifespan", 20.0f);
		Vector3 missileBound = {arenaSize * 2, arenaSize * 2, arenaSize * 2};

		entt::entity missile = context.templateReg.create();
		t_model_id model = context.modelManager.loadModel("assets/Models/missile/missile.glb");
		context.templateReg.emplace<tag::VelocitySyncModelRot>(missile);
		context.templateReg.emplace<CollisionBody>(missile, CollisionBody{rad});
		context.templateReg.emplace<RenderBody>(missile, RenderBody{model, color, rad});
		context.templateReg.emplace<DisappearBound>(missile, missileBound * -1, missileBound);
		context.templateReg.emplace<SpawnsTrailParticle>(missile, SpawnsTrailParticle{rad * 0.5f, 0.5f});
		context.templateReg.emplace<Rotation>(missile);
		context.templateReg.emplace<tag::VelocitySyncRot>(missile);
		context.templateReg.emplace<MoveTarget>(missile);
		context.templateReg.emplace<tag::AIMoveControl>(missile);
		context.templateReg.emplace<tag::Suicidal>(missile);
		context.templateReg.emplace<tag::Missile>(missile);
		context.templateReg.emplace<Lifespan>(missile, Lifespan{lifespan});
		context.templateReg.emplace<sound::DeathSound>(missile, sound::RANDOM_EXPLOSION, std::min(1.0f, rad / 1.0f * 0.5f));
		return missile;
	}

	float getBaseSpread(const GameConfig& cfg) {
		float combatDist = cfg.getFloat("game.combatDist", 1000.0f);
		float rangeMultiplier = cfg.getFloat("weapons.missile.effectiveRangeMultiplier", 5.0f);
		float effectiveRange = combatDist * rangeMultiplier;
		return std::atan2(1.0f, effectiveRange);
	}
}

void weapon::emplaceWeaponMissileBasic(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.missile.basic.";

	float baseSpeed = cfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseDamage = cfg.getFloat("weapons.missile.baseDamage", 250.0f);
	float baseSpread = getBaseSpread(cfg);

	float radius = cfg.getFloat(path + "radius", 0.5f);
	float hp = cfg.getFloat(path + "hp", 1.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 1.0f);
	float acceleration = cfg.getFloat(path + "acceleration", 100.0f);
	float turnSpeed = cfg.getFloat(path + "turnSpeed", 0.5f);
	int ammo = cfg.getInt(path + "ammo", 2);
	float ammoRegen = cfg.getFloat(path + "ammoRegen", 0.0667f);
	float cooldown = cfg.getFloat(path + "cooldown", 1.0f);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<tag::effect::ExplodeOnDeath>(bulletTemplate);
	context.templateReg.emplace<ScalarAcceleration>(bulletTemplate, ScalarAcceleration{acceleration});
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = baseSpeed;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{ammoRegen});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
}

void weapon::emplaceWeaponMissileSwarm(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.missile.swarm.";

	float baseSpeed = cfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseLifespan = cfg.getFloat("weapons.missile.lifespan", 20.0f);

	float radius = cfg.getFloat(path + "radius", 0.15f);
	float hp = cfg.getFloat(path + "hp", 1.0f);
	float turnSpeed = cfg.getFloat(path + "turnSpeed", 0.75f);
	float lifespanMultiplier = cfg.getFloat(path + "lifespanMultiplier", 0.5f);
	float spreadAngle = cfg.getFloat(path + "spreadAngle", 0.785f);
	int bulletCount = cfg.getInt(path + "bulletCount", 4);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 2.0f);
	int ammo = cfg.getInt(path + "ammo", 3);
	float reloadTime = cfg.getFloat(path + "reloadTime", 15.0f);
	float cooldown = cfg.getFloat(path + "cooldown", 0.25f);
	float extendFireRequest = cfg.getFloat(path + "extendFireRequest", 2.0f);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<tag::effect::ExplodeOnDeath>(bulletTemplate);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});
	context.templateReg.emplace_or_replace<Lifespan>(bulletTemplate, Lifespan{baseLifespan * lifespanMultiplier});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(spreadAngle);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});
}

void weapon::emplaceWeaponMissileTorpedo(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.missile.torpedo.";

	float baseDamage = cfg.getFloat("weapons.missile.baseDamage", 250.0f);
	float maxSpeed = cfg.getFloat("weapons.missile.maxSpeed", 400.0f);
	float baseSpread = getBaseSpread(cfg);

	float radius = cfg.getFloat(path + "radius", 0.25f);
	float hp = cfg.getFloat(path + "hp", 1.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 0.25f);
	float turnSpeed = cfg.getFloat(path + "turnSpeed", 0.1f);
	int ammo = cfg.getInt(path + "ammo", 4);
	float reloadTime = cfg.getFloat(path + "reloadTime", 20.0f);
	float cooldown = cfg.getFloat(path + "cooldown", 0.2f);
	float extendFireRequest = cfg.getFloat(path + "extendFireRequest", 2.0f);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<tag::effect::ExplodeOnDeath>(bulletTemplate);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = maxSpeed;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});
}

void weapon::emplaceWeaponMissileNuke(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.missile.nuke.";

	float baseSpeed = cfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseDamage = cfg.getFloat("weapons.missile.baseDamage", 250.0f);
	float baseSpread = getBaseSpread(cfg);

	float radius = cfg.getFloat(path + "radius", 2.0f);
	float hp = cfg.getFloat(path + "hp", 250.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 10.0f);
	float turnSpeed = cfg.getFloat(path + "turnSpeed", 1.5f);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 0.5f);
	float delayedDamageTime = cfg.getFloat(path + "delayedDamageTime", 40.0f);
	float delayedDamage = cfg.getFloat(path + "delayedDamage", 1000000.0f);
	int ammo = cfg.getInt(path + "ammo", 1);
	float reloadTime = cfg.getFloat(path + "reloadTime", 30.0f);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<tag::effect::ExplodeOnDeath>(bulletTemplate);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});
	context.templateReg.remove<Lifespan>(bulletTemplate);
	context.templateReg.emplace<DelayedDamage>(bulletTemplate, DelayedDamage{delayedDamageTime, delayedDamage});
	
	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
}

void weapon::emplaceWeaponMissileSniper(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.missile.sniper.";

	float radius = cfg.getFloat(path + "radius", 0.5f);
	float hp = cfg.getFloat(path + "hp", 1.0f);
	float speed = cfg.getFloat(path + "speed", 1000.0f);
	float cooldown = cfg.getFloat(path + "cooldown", 2.0f);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<tag::effect::ExplodeOnDeath>(bulletTemplate);

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = 0.0f;
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = speed;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
}

void weapon::emplaceWeaponMissileFlares(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.missile.flares.";

	float baseSpeed = cfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseLifespan = cfg.getFloat("weapons.missile.lifespan", 20.0f);

	float radius = cfg.getFloat(path + "radius", 0.15f);
	float hp = cfg.getFloat(path + "hp", 1.0f);
	float turnSpeed = cfg.getFloat(path + "turnSpeed", 0.05f);
	float lifespanMultiplier = cfg.getFloat(path + "lifespanMultiplier", 0.2f);
	float spreadAngle = cfg.getFloat(path + "spreadAngle", 1.571f);
	int bulletCount = cfg.getInt(path + "bulletCount", 2);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 0.5f);
	int ammo = cfg.getInt(path + "ammo", 4);
	float ammoReload = cfg.getFloat(path + "ammoReload", 8.0f);
	float cooldown = cfg.getFloat(path + "cooldown", 0.2f);
	float extendFireRequest = cfg.getFloat(path + "extendFireRequest", 1.0f);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<tag::Targetable>(bulletTemplate);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});
	context.templateReg.emplace_or_replace<Lifespan>(bulletTemplate, Lifespan{baseLifespan * lifespanMultiplier});
	context.templateReg.emplace_or_replace<SpawnsTrailParticle>(bulletTemplate, SpawnsTrailParticle{radius, 0.5f, ORANGE});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(spreadAngle);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{ammoReload});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});
}