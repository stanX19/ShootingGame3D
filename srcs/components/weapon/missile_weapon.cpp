#include "weapons.hpp"
#include "utils.hpp"
#include "components/sound.hpp"
#include "game_config.hpp"
#include <algorithm>

namespace
{
	const Color BASE_COLOR = GRAY;
	const Color NUKE_EXPLOSION_COLOR = {0, 255, 255, 255};
	const float DEFAULT_MASS = 5.0f;

	Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity, Color baseColor = BASE_COLOR)
	{
		return baseColor;
	}

	void emplaceMissileWeaponCommon(GameContext &context, entt::entity entity, sound::Id shootSoundId = sound::RANDOM_MISSILE_SHOOT)
	{
		context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
		context.registry.emplace_or_replace<AimTarget>(entity);
		context.registry.emplace_or_replace<AimDirection>(entity);
		context.registry.emplace_or_replace<sound::ShootSound>(entity, shootSoundId, 0.5f);
	}

	float getDefaultInstantDamage(const GameConfig &globalCfg, const GameConfig &cfg, float legacyMultiplier)
	{
		const float baseDamage = globalCfg.getFloat(
			"weapons.missile.instantDamage",
			globalCfg.getFloat("weapons.missile.baseDamage", 250.0f)
		);
		return cfg.getFloat(
			"instantDamage",
			baseDamage * cfg.getFloat("damageMultiplier", legacyMultiplier)
		);
	}

	void emplaceMissileDeathEffects(
		GameContext &context,
		entt::entity bulletTemplate,
		const GameConfig &cfg,
		float radius,
		float defaultInstantDamage,
		Color explosionColor = effect::EXPLOSION_COLOR
	)
	{
		const float defaultExplosionStartRadius = radius * 0.5f;
		// Keep pulse damage and visual growth independent. Rule of thumb: a missile with
		// a large instant AOE should usually start its visible explosion near that radius;
		// small missiles can start smaller to preserve gradual visual expansion.
		const float instantRadius = std::max(0.0f, cfg.getFloat("instantRadius", defaultExplosionStartRadius));
		const float explosionStartRadius = std::max(
			0.0f,
			cfg.getFloat("explosionStartRadius", defaultExplosionStartRadius)
		);

		context.templateReg.emplace<effect::InstantDamageOnDeath>(
			bulletTemplate,
			effect::InstantDamageOnDeath{
				defaultInstantDamage,
				instantRadius
			}
		);
		context.templateReg.emplace<effect::ExplodeOnDeath>(
			bulletTemplate,
			effect::ExplodeOnDeath{
				cfg.getFloat("explosionFinalRadius", radius * 10.0f),
				explosionStartRadius,
				cfg.getFloat("explosionDuration", effect::DEFAULT_EXPLOSION_DURATION),
				cfg.getFloat("explosionDamage", effect::DEFAULT_EXPLOSION_DAMAGE),
				explosionColor
			}
		);
	}

	entt::entity createMissileTemplate(GameContext &context, float rad, Color color, float mass)
	{
		const auto &cfg = context.config;
		float arenaSize = cfg.getFloat("game.arenaSize", 2000.0f);
		float lifespan = cfg.getFloat("weapons.missile.lifespan", 20.0f);
		float bodyDamage = cfg.getFloat("weapons.missile.bodyDamage", 2.5f);
		Vector3 missileBound = {arenaSize * 2, arenaSize * 2, arenaSize * 2};

		entt::entity missile = context.templateReg.create();
		t_model_id model = context.modelManager.loadModel("assets/Models/missile/missile.glb");
		context.templateReg.emplace<tag::VelocitySyncModelRot>(missile);
		context.templateReg.emplace<CollisionBody>(missile, CollisionBody{rad});
		context.templateReg.emplace<Damage>(missile, Damage{bodyDamage});
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
		context.templateReg.emplace<Mass>(missile, mass);
		return missile;
	}

	float getBaseSpread(const GameConfig &cfg)
	{
		float combatDist = cfg.getFloat("game.combatDist", 1000.0f);
		float rangeMultiplier = cfg.getFloat("weapons.missile.effectiveRangeMultiplier", 5.0f);
		float effectiveRange = combatDist * rangeMultiplier;
		return std::atan2(1.0f, effectiveRange);
	}
}

void weapon::emplaceGenericMissile(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.5f);
	float hp = cfg.getFloat("hp", 1.0f);
	float instantDamage = getDefaultInstantDamage(globalCfg, cfg, 1.0f);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 1.0f);
	float turnSpeed = cfg.getFloat("turnSpeed", 0.5f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float cooldown = cfg.getFloat("cooldown", 1.0f);
	float mass = cfg.getFloat("mass", DEFAULT_MASS);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity), mass);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	emplaceMissileDeathEffects(context, bulletTemplate, cfg, radius, instantDamage);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});

	if (float acceleration = cfg.getFloat("acceleration", 0.0f); acceleration > 0.0f)
		context.templateReg.emplace<ScalarAcceleration>(bulletTemplate, ScalarAcceleration{acceleration});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_MISSILE_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	if (int ammo = cfg.getInt("ammo", 0); ammo > 0)
	{
		context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
		if (float ammoRegen = cfg.getFloat("ammoRegen", 0.0f); ammoRegen > 0.0f)
			context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{ammoRegen});
		if (float ammoReload = cfg.getFloat("reloadTime", cfg.getFloat("ammoReload", 0.0f)); ammoReload > 0.0f)
			context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{ammoReload});
	}

	if (float extendFireRequest = cfg.getFloat("extendFireRequest", 0.0f); extendFireRequest > 0.0f)
		context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Generic Missile"));
}

void weapon::emplaceWeaponMissileBasic(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.missile.basic.";

	float baseSpeed = globalCfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.5f);
	float hp = cfg.getFloat("hp", 1.0f);
	float instantDamage = getDefaultInstantDamage(globalCfg, cfg, 1.0f);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 1.0f);
	float acceleration = cfg.getFloat("acceleration", 100.0f);
	float turnSpeed = cfg.getFloat("turnSpeed", 0.5f);
	int ammo = cfg.getInt("ammo", 2);
	float ammoRegen = cfg.getFloat("ammoRegen", 0.0667f);
	float cooldown = cfg.getFloat("cooldown", 1.0f);
	float mass = cfg.getFloat("mass", DEFAULT_MASS);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity), mass);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	emplaceMissileDeathEffects(context, bulletTemplate, cfg, radius, instantDamage);
	context.templateReg.emplace<ScalarAcceleration>(bulletTemplate, ScalarAcceleration{acceleration});
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_MISSILE_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{ammoRegen});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Missile Basic"));
}

void weapon::emplaceWeaponMissileSwarm(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.missile.swarm.";

	float baseSpeed = globalCfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseLifespan = globalCfg.getFloat("weapons.missile.lifespan", 20.0f);

	float radius = cfg.getFloat("radius", 0.15f);
	float hp = cfg.getFloat("hp", 1.0f);
	float turnSpeed = cfg.getFloat("turnSpeed", 0.75f);
	float lifespanMultiplier = cfg.getFloat("lifespanMultiplier", 0.5f);
	float spreadAngle = cfg.getFloat("spreadAngle", 0.785f);
	int bulletCount = cfg.getInt("bulletCount", 4);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 2.0f);
	int ammo = cfg.getInt("ammo", 3);
	float reloadTime = cfg.getFloat("reloadTime", 15.0f);
	float cooldown = cfg.getFloat("cooldown", 0.25f);
	float extendFireRequest = cfg.getFloat("extendFireRequest", 2.0f);
	float mass = cfg.getFloat("mass", DEFAULT_MASS);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity), mass);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	emplaceMissileDeathEffects(
		context,
		bulletTemplate,
		cfg,
		radius,
		getDefaultInstantDamage(globalCfg, cfg, 0.0f)
	);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});
	context.templateReg.emplace_or_replace<Lifespan>(bulletTemplate, Lifespan{baseLifespan * lifespanMultiplier});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(spreadAngle);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_MISSILE_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Missile Swarm"));
}

void weapon::emplaceWeaponMissileTorpedo(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.missile.torpedo.";

	float baseSpeed = globalCfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.25f);
	float hp = cfg.getFloat("hp", 1.0f);
	float instantDamage = getDefaultInstantDamage(globalCfg, cfg, 0.25f);
	float turnSpeed = cfg.getFloat("turnSpeed", 0.1f);
	int ammo = cfg.getInt("ammo", 4);
	float reloadTime = cfg.getFloat("reloadTime", 20.0f);
	float cooldown = cfg.getFloat("cooldown", 0.2f);
	float extendFireRequest = cfg.getFloat("extendFireRequest", 2.0f);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 2.0f);
	float mass = cfg.getFloat("mass", DEFAULT_MASS);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity), mass);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	emplaceMissileDeathEffects(context, bulletTemplate, cfg, radius, instantDamage);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_MISSILE_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Torpedo"));
}

void weapon::emplaceWeaponMissileNuke(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.missile.nuke.";

	float baseSpeed = globalCfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 2.0f);
	float hp = cfg.getFloat("hp", 250.0f);
	float instantDamage = getDefaultInstantDamage(globalCfg, cfg, 10.0f);
	float turnSpeed = cfg.getFloat("turnSpeed", 1.5f);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 0.5f);
	float delayedDamageTime = cfg.getFloat("delayedDamageTime", 40.0f);
	float delayedDamage = cfg.getFloat("delayedDamage", 1000000.0f);
	int ammo = cfg.getInt("ammo", 1);
	float reloadTime = cfg.getFloat("reloadTime", 30.0f);
	float mass = cfg.getFloat("mass", DEFAULT_MASS);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity), mass);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	emplaceMissileDeathEffects(context, bulletTemplate, cfg, radius, instantDamage, NUKE_EXPLOSION_COLOR);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});
	context.templateReg.remove<Lifespan>(bulletTemplate);
	context.templateReg.emplace<DelayedDamage>(bulletTemplate, DelayedDamage{delayedDamageTime, delayedDamage});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_MISSILE_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Nuke"));
}

void weapon::emplaceWeaponMissileSniper(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.missile.sniper.";

	float baseSpeed = globalCfg.getFloat("weapons.missile.baseSpeed", 100.0f);

	float radius = cfg.getFloat("radius", 0.5f);
	float hp = cfg.getFloat("hp", 1.0f);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 10.0f);
	float cooldown = cfg.getFloat("cooldown", 2.0f);
	float spreadAngle = cfg.getFloat("spreadAngle", 0.0f);
	float mass = cfg.getFloat("mass", DEFAULT_MASS);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity), mass);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	emplaceMissileDeathEffects(
		context,
		bulletTemplate,
		cfg,
		radius,
		getDefaultInstantDamage(globalCfg, cfg, 0.0f)
	);

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(spreadAngle);
	weapon.bulletData.bulletCount = 1;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_MISSILE_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Missile Sniper"));
}

void weapon::emplaceWeaponMissileFlares(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.missile.flares.";

	float baseSpeed = globalCfg.getFloat("weapons.missile.baseSpeed", 100.0f);
	float baseLifespan = globalCfg.getFloat("weapons.missile.lifespan", 20.0f);

	float radius = cfg.getFloat("radius", 0.15f);
	float hp = cfg.getFloat("hp", 1.0f);
	float turnSpeed = cfg.getFloat("turnSpeed", 0.05f);
	float lifespanMultiplier = cfg.getFloat("lifespanMultiplier", 0.2f);
	float spreadAngle = cfg.getFloat("spreadAngle", 1.571f);
	int bulletCount = cfg.getInt("bulletCount", 2);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 0.5f);
	int ammo = cfg.getInt("ammo", 4);
	float ammoReload = cfg.getFloat("ammoReload", 8.0f);
	float cooldown = cfg.getFloat("cooldown", 0.2f);
	float extendFireRequest = cfg.getFloat("extendFireRequest", 1.0f);
	float mass = cfg.getFloat("mass", DEFAULT_MASS);

	entt::entity bulletTemplate = createMissileTemplate(context, radius, getColor(context, entity), mass);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<tag::Targetable>(bulletTemplate);
	context.templateReg.emplace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});
	context.templateReg.emplace_or_replace<Lifespan>(bulletTemplate, Lifespan{baseLifespan * lifespanMultiplier});
	context.templateReg.emplace_or_replace<SpawnsTrailParticle>(bulletTemplate, SpawnsTrailParticle{radius, 0.5f, ORANGE});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(spreadAngle);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceMissileWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_MISSILE_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{ammoReload});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Flares"));
}
