#include "weapons.hpp"
#include "utils.hpp"
#include "components/sound.hpp"
#include "game_config.hpp"

namespace
{
	const Color BASE_COLOR = GREEN;

	Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity, Color baseColor = BASE_COLOR)
	{
		return baseColor;
	}

	const float DEFAULT_MASS = 0.0f;

	t_model_id getBulletModel(GameContext &context)
	{
		std::string path = context.config.getString("weapons.lazer.modelPath", "");
		if (!path.empty())
			return context.modelManager.loadModel(path);
		return context.modelManager.createCylinder();
	}

	void emplaceLazerWeaponCommon(GameContext &context, entt::entity entity, sound::Id shootSoundId = sound::RANDOM_LAZER_SHOOT)
	{
		context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
		context.registry.emplace_or_replace<AimTarget>(entity);
		context.registry.emplace_or_replace<AimDirection>(entity);
		context.registry.emplace_or_replace<sound::ShootSound>(entity, shootSoundId, 0.4f);
	}

	entt::entity createBulletTemplate(GameContext &context, float rad, Color color)
	{
		const auto &cfg = context.config;
		float arenaSize = cfg.getFloat("game.arenaSize", 2000.0f);
		Vector3 lazerBound = {arenaSize * 2, arenaSize * 2, arenaSize * 2};

		entt::entity bullet = context.templateReg.create();
		t_model_id model = getBulletModel(context);
		context.templateReg.emplace<tag::Bullet>(bullet);
		context.templateReg.emplace<tag::VelocitySyncModelRot>(bullet);
		context.templateReg.emplace<tag::bullet_type::Energy>(bullet);
		context.templateReg.emplace<tag::bullet_type::Lazer>(bullet);
		context.templateReg.emplace<ModelStrech>(bullet, 1.0f / (2 * rad));
		context.templateReg.emplace<CollisionBody>(bullet, CollisionBody{rad});
		context.templateReg.emplace<RenderBody>(bullet, RenderBody{model, color, rad});
		context.templateReg.emplace<DisappearBound>(bullet, lazerBound * -1, lazerBound);
		context.templateReg.emplace<sound::HitSound>(bullet, sound::RANDOM_LAZER_HIT, 0.3f);
		return bullet;
	}

	float getBaseSpread(const GameConfig &cfg)
	{
		float combatDist = cfg.getFloat("game.combatDist", 1000.0f);
		float rangeMultiplier = cfg.getFloat("weapons.lazer.effectiveRangeMultiplier", 1.0f);
		float effectiveRange = combatDist * rangeMultiplier;
		return std::atan2(1.0f, effectiveRange);
	}
}

void weapon::emplaceGenericLazer(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.lazer.baseSpeed", 100000.0f);
	float baseDamage = globalCfg.getFloat("weapons.lazer.baseDamage", 10.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.05f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 1.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float cooldown = cfg.getFloat("cooldown", 0.4f);

	entt::entity bulletTemplate = createBulletTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<Mass>(bulletTemplate, cfg.getFloat("mass", DEFAULT_MASS));

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed;

	emplaceLazerWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_LAZER_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	if (int ammo = cfg.getInt("ammo", 0); ammo > 0)
	{
		context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
		if (float ammoRegen = cfg.getFloat("ammoRegen", 0.0f); ammoRegen > 0.0f)
			context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{ammoRegen});
	}

	if (float extendFireRequest = cfg.getFloat("extendFireRequest", 0.0f); extendFireRequest > 0.0f)
		context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});

	if (float chargeTime = cfg.getFloat("chargeTime", 0.0f); chargeTime > 0.0f)
		context.registry.emplace_or_replace<ChargedWeapon>(entity, ChargedWeapon{chargeTime, ColorAlpha(getColor(context, entity), 0.5f)});

	if (float extendFireDuration = cfg.getFloat("extendFireDuration", 0.0f); extendFireDuration > 0.0f)
		context.registry.emplace_or_replace<ExtendFireDuration>(entity, ExtendFireDuration{extendFireDuration});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Generic Lazer"));
}

void weapon::emplaceWeaponLazerBasic(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.lazer.basic.";

	float baseSpeed = globalCfg.getFloat("weapons.lazer.baseSpeed", 100000.0f);
	float baseDamage = globalCfg.getFloat("weapons.lazer.baseDamage", 10.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.05f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 2.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 1.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float cooldown = cfg.getFloat("cooldown", 0.4f);

	entt::entity bulletTemplate = createBulletTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<Mass>(bulletTemplate, cfg.getFloat("mass", DEFAULT_MASS));

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed;

	emplaceLazerWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_LAZER_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Lazer Basic"));
}

void weapon::emplaceWeaponLazerMachineGun(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.lazer.machineGun.";

	float baseSpeed = globalCfg.getFloat("weapons.lazer.baseSpeed", 100000.0f);
	float baseDamage = globalCfg.getFloat("weapons.lazer.baseDamage", 10.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.05f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 0.5f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 5.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	int ammo = cfg.getInt("ammo", 80);
	float ammoRegen = cfg.getFloat("ammoRegen", 6.0f);
	float cooldown = cfg.getFloat("cooldown", 0.05f);
	float extendFireRequest = cfg.getFloat("extendFireRequest", 0.5f);

	entt::entity bulletTemplate = createBulletTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<Mass>(bulletTemplate, cfg.getFloat("mass", DEFAULT_MASS));

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed;

	emplaceLazerWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_LAZER_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo + 40)});
	context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{ammoRegen});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Lazer Machine Gun"));
}

void weapon::emplaceWeaponLazerDeletor(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.lazer.deletor.";

	float baseSpeed = globalCfg.getFloat("weapons.lazer.baseSpeed", 100000.0f);
	float baseDamage = globalCfg.getFloat("weapons.lazer.baseDamage", 10.0f);

	float radius = cfg.getFloat("radius", 0.5f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 0.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	int ammo = cfg.getInt("ammo", 1);
	float ammoRegen = cfg.getFloat("ammoRegen", 0.125f);
	float extendFireRequest = cfg.getFloat("extendFireRequest", 2.0f);
	float chargeTime = cfg.getFloat("chargeTime", 1.5f);
	float extendFireDuration = cfg.getFloat("extendFireDuration", 1.5f);
	Color color = getColor(context, entity);

	entt::entity bulletTemplate = createBulletTemplate(context, radius, color);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<Mass>(bulletTemplate, cfg.getFloat("mass", DEFAULT_MASS));

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = spreadMultiplier;
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed;

	emplaceLazerWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_LAZER_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{ammoRegen});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});
	context.registry.emplace_or_replace<ChargedWeapon>(entity, ChargedWeapon{chargeTime, ColorAlpha(color, 0.5f)});
	context.registry.emplace_or_replace<ExtendFireDuration>(entity, ExtendFireDuration{extendFireDuration});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Lazer Deletor"));
}

void weapon::emplaceWeaponLazerShotgun(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;
	const std::string path = "weapons.lazer.shotgun.";

	float baseSpeed = globalCfg.getFloat("weapons.lazer.baseSpeed", 100000.0f);
	float baseDamage = globalCfg.getFloat("weapons.lazer.baseDamage", 10.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.1f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 25.0f);
	int bulletCount = cfg.getInt("bulletCount", 20);
	int ammo = cfg.getInt("ammo", 5);
	float ammoRegen = cfg.getFloat("ammoRegen", 1.0f);
	float cooldown = cfg.getFloat("cooldown", 0.5f);

	entt::entity bulletTemplate = createBulletTemplate(context, radius, getColor(context, entity));
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<Mass>(bulletTemplate, cfg.getFloat("mass", DEFAULT_MASS));

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed;

	emplaceLazerWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_LAZER_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{ammoRegen});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Lazer Shotgun"));
}
