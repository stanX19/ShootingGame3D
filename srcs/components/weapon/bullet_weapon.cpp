#include "weapons.hpp"
#include "utils.hpp"
#include "factions.hpp"
#include "components/sound.hpp"
#include "game_config.hpp"
#include <string>

namespace
{
	Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity, Color baseColor = WHITE)
	{
		return baseColor;
	}

	void emplaceBulletWeaponCommon(GameContext &context, entt::entity entity, sound::Id shootSoundId = sound::RANDOM_BULLET_SHOOT)
	{
		context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
		context.registry.emplace_or_replace<AimTarget>(entity);
		context.registry.emplace_or_replace<AimDirection>(entity);
		context.registry.emplace_or_replace<sound::ShootSound>(entity, shootSoundId, 0.5f);
	}

	entt::entity createBulletTemplate(GameContext &context)
	{
		const auto &cfg = context.config;
		Vector3 bulletBound = {cfg.ARENA_SIZE + cfg.COMBAT_DIST * 2, cfg.ARENA_SIZE + cfg.COMBAT_DIST * 2, cfg.ARENA_SIZE + cfg.COMBAT_DIST * 2};

		entt::entity bullet = context.templateReg.create();
		context.templateReg.emplace<tag::Bullet>(bullet);
		context.templateReg.emplace<faction::Faction>(bullet, faction::FAC_BULLET);
		context.templateReg.emplace<tag::VelocitySyncModelRot>(bullet);
		context.templateReg.emplace<tag::bullet_type::Kinetic>(bullet);
		context.templateReg.emplace<ModelStrech>(bullet, 1.0f);
		context.templateReg.emplace<DisappearBound>(bullet, bulletBound * -1, bulletBound);
		context.templateReg.emplace<sound::HitSound>(bullet, sound::RANDOM_BULLET_HIT, 0.4f);
		return bullet;
	}

	float getBaseSpread(const GameConfig &cfg)
	{
		float rangeMultiplier = cfg.getFloat("weapons.bullet.effectiveRangeMultiplier", 2.0f);
		float effectiveRange = cfg.COMBAT_DIST * rangeMultiplier;
		return std::atan2(1.0f, effectiveRange);
	}

	t_model_id getBulletModel(GameContext &context)
	{
		std::string path = context.config.getString("weapons.bullet.modelPath", "");
		if (!path.empty())
			return context.modelManager.loadModel(path);
		return context.modelManager.createSphere();
	}
}

void weapon::emplaceGenericBullet(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = globalCfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.05f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 1.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 1.0f);
	float lifespan = cfg.getFloat("lifespan", 10.0f);

	t_model_id model = getBulletModel(context);

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{lifespan});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceBulletWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_BULLET_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);

	if (float cooldown = cfg.getFloat("cooldown", -1.0f); cooldown > 0)
		context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	if (int ammo = cfg.getInt("ammo", -1); ammo > 0)
		context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});

	if (float reloadTime = cfg.getFloat("reloadTime", -1.0f); reloadTime > 0)
		context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Generic Bullet"));
}

void weapon::emplaceWeaponMachineGun(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = globalCfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.05f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 10.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 2.0f);
	float lifespan = cfg.getFloat("lifespan", 10.0f);
	int ammo = cfg.getInt("ammo", 80);
	float reloadTime = cfg.getFloat("reloadTime", 5.5f);
	float cooldown = cfg.getFloat("cooldown", 0.1f);

	t_model_id model = getBulletModel(context);

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{lifespan});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceBulletWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_BULLET_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Machine Gun"));
}

void weapon::emplaceWeaponShotgun(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = globalCfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.025f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 100.0f);
	int bulletCount = cfg.getInt("bulletCount", 10);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 2.0f);
	float lifespan = cfg.getFloat("lifespan", 5.0f);
	float modelStretch = cfg.getFloat("modelStretch", 0.5f);
	int ammo = cfg.getInt("ammo", 5);
	float reloadTime = cfg.getFloat("reloadTime", 1.5f);
	float cooldown = cfg.getFloat("cooldown", 0.1f);
	float extendFireRequest = cfg.getFloat("extendFireRequest", 1.0f);

	t_model_id model = getBulletModel(context);

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{lifespan});
	context.templateReg.emplace_or_replace<ModelStrech>(bulletTemplate, ModelStrech{modelStretch / (radius * 2)});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceBulletWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_BULLET_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Shotgun"));
}

void weapon::emplaceWeaponBigBall(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = globalCfg.getFloat("weapons.bullet.baseDamage", 25.0f);

	float radius = cfg.getFloat("radius", 1.5f);
	float hp = cfg.getFloat("hp", 1000.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 15.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 0.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 0.25f);
	float lifespan = cfg.getFloat("lifespan", 15.0f);
	int ammo = cfg.getInt("ammo", 1);
	float reloadTime = cfg.getFloat("reloadTime", 7.0f);
	float extendFireRequest = cfg.getFloat("extendFireRequest", 2.0f);
	float chargeTime = cfg.getFloat("chargeTime", 1.0f);

	t_model_id model = context.modelManager.loadModel("assets/Models/asteroid/asteroid_ceres.glb", Vector3{0.36f, 0.36f, 0.38f}, Vector3UnitZ, Vector3{0.5f, 0.75f, 0.5f});

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{lifespan});
	context.templateReg.emplace<Rotation>(bulletTemplate);
	context.templateReg.emplace<RotationVelocity>(bulletTemplate, QuaternionFromAxisAngle(Vector3UnitY, PI));
	context.templateReg.erase<ModelStrech>(bulletTemplate);

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = spreadMultiplier;
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceBulletWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_BULLET_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0.0f, static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});
	context.registry.emplace_or_replace<ChargedWeapon>(entity, ChargedWeapon{chargeTime, ColorAlpha(WHITE, 0.25f)});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Big Ball"));
}

void weapon::emplaceWeaponSniper(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = globalCfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.2f);
	float hp = cfg.getFloat("hp", 50.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 2.5f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 0.01f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 6.0f);
	float lifespan = cfg.getFloat("lifespan", 10.0f);
	float cooldown = cfg.getFloat("cooldown", 1.5f);

	t_model_id model = getBulletModel(context);

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{lifespan});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceBulletWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_BULLET_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Sniper"));
}

void weapon::emplaceWeaponBurstSniper(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = globalCfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.2f);
	float hp = cfg.getFloat("hp", 50.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 2.5f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 5.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 6.0f);
	float lifespan = cfg.getFloat("lifespan", 10.0f);
	float cooldown = cfg.getFloat("cooldown", 0.35f);
	int ammo = cfg.getInt("ammo", 20);
	float reloadTime = cfg.getFloat("reloadTime", 12.0f);

	t_model_id model = getBulletModel(context);

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{lifespan});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceBulletWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_BULLET_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Burst Sniper"));
}

void weapon::emplaceWeaponBasic(GameContext &context, entt::entity entity, const GameConfig &cfg)
{
	const auto &globalCfg = context.config;

	float baseSpeed = globalCfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = globalCfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(globalCfg);

	float radius = cfg.getFloat("radius", 0.075f);
	float hp = cfg.getFloat("hp", 1.0f);
	float damageMultiplier = cfg.getFloat("damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat("spreadMultiplier", 5.0f);
	int bulletCount = cfg.getInt("bulletCount", 1);
	float speedMultiplier = cfg.getFloat("speedMultiplier", 1.2f);
	float lifespan = cfg.getFloat("lifespan", 10.0f);
	float cooldown = cfg.getFloat("cooldown", 0.25f);
	int ammo = cfg.getInt("ammo", 30);
	float reloadTime = cfg.getFloat("reloadTime", 2.0f);

	t_model_id model = getBulletModel(context);

	entt::entity bulletTemplate = createBulletTemplate(context);
	context.templateReg.emplace<HP>(bulletTemplate, HP{hp});
	context.templateReg.emplace<Damage>(bulletTemplate, Damage{baseDamage * damageMultiplier});
	context.templateReg.emplace<CollisionBody>(bulletTemplate, CollisionBody{radius});
	context.templateReg.emplace<RenderBody>(bulletTemplate, RenderBody{model, getColor(context, entity), radius});
	context.templateReg.emplace<Lifespan>(bulletTemplate, Lifespan{lifespan});

	Weapon weapon{bulletTemplate};
	weapon.bulletData.spreadSin = std::sin(baseSpread * spreadMultiplier);
	weapon.bulletData.bulletCount = bulletCount;
	weapon.bulletData.speed = baseSpeed * speedMultiplier;

	emplaceBulletWeaponCommon(context, entity, cfg.getString("sound", "").empty() ? sound::RANDOM_BULLET_SHOOT : context.soundManager.loadSound(cfg.getString("sound", "")));
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});

	context.registry.emplace_or_replace<WeaponName>(entity, cfg.getString("name", "Gunner"));
}
