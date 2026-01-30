#include "weapons.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "factions.hpp"
#include "components/sound.hpp"
#include "game_config.hpp"

namespace
{
	Color getColor([[maybe_unused]] GameContext &context, [[maybe_unused]] entt::entity entity, Color baseColor = WHITE)
	{
		return baseColor;
	}

	void emplaceBulletWeaponCommon(GameContext &context, entt::entity entity) {
		context.registry.emplace_or_replace<tag::weapon::IsWeapon>(entity);
		context.registry.emplace_or_replace<AimTarget>(entity);
		context.registry.emplace_or_replace<AimDirection>(entity);
		context.registry.emplace_or_replace<sound::ShootSound>(entity, sound::RANDOM_BULLET_SHOOT, 0.5f);
	}

	entt::entity createBulletTemplate(GameContext &context) {
		const auto& cfg = context.config;
		float arenaSize = cfg.getFloat("game.arenaSize", 2000.0f);
		float combatDist = cfg.getFloat("game.combatDist", 1000.0f);
		Vector3 bulletBound = {arenaSize + combatDist * 2, arenaSize + combatDist * 2, arenaSize + combatDist * 2};

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

	float getBaseSpread(const GameConfig& cfg) {
		float combatDist = cfg.getFloat("game.combatDist", 1000.0f);
		float rangeMultiplier = cfg.getFloat("weapons.bullet.effectiveRangeMultiplier", 2.0f);
		float effectiveRange = combatDist * rangeMultiplier;
		return std::atan2(1.0f, effectiveRange);
	}
}

void weapon::emplaceWeaponMachineGun(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.bullet.machineGun.";

	float baseSpeed = cfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = cfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(cfg);

	float radius = cfg.getFloat(path + "radius", 0.05f);
	float hp = cfg.getFloat(path + "hp", 1.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat(path + "spreadMultiplier", 10.0f);
	int bulletCount = cfg.getInt(path + "bulletCount", 1);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 2.0f);
	float lifespan = cfg.getFloat(path + "lifespan", 10.0f);
	int ammo = cfg.getInt(path + "ammo", 80);
	float reloadTime = cfg.getFloat(path + "reloadTime", 5.5f);
	float cooldown = cfg.getFloat(path + "cooldown", 0.1f);

	t_model_id model = context.modelManager.createSphere();
	
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

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
}

void weapon::emplaceWeaponShotgun(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.bullet.shotgun.";

	float baseSpeed = cfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = cfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(cfg);

	float radius = cfg.getFloat(path + "radius", 0.025f);
	float hp = cfg.getFloat(path + "hp", 1.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat(path + "spreadMultiplier", 100.0f);
	int bulletCount = cfg.getInt(path + "bulletCount", 10);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 2.0f);
	float lifespan = cfg.getFloat(path + "lifespan", 5.0f);
	float modelStretch = cfg.getFloat(path + "modelStretch", 0.5f);
	int ammo = cfg.getInt(path + "ammo", 5);
	float reloadTime = cfg.getFloat(path + "reloadTime", 1.5f);
	float cooldown = cfg.getFloat(path + "cooldown", 0.1f);
	float extendFireRequest = cfg.getFloat(path + "extendFireRequest", 1.0f);

	t_model_id model = context.modelManager.createSphere();

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

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireRequest});
}

void weapon::emplaceWeaponBigBall(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.bullet.bigBall.";

	float baseSpeed = cfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = cfg.getFloat("weapons.bullet.baseDamage", 25.0f);

	float radius = cfg.getFloat(path + "radius", 1.5f);
	float hp = cfg.getFloat(path + "hp", 1000.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 15.0f);
	float spreadMultiplier = cfg.getFloat(path + "spreadMultiplier", 0.0f);
	int bulletCount = cfg.getInt(path + "bulletCount", 1);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 0.25f);
	float lifespan = cfg.getFloat(path + "lifespan", 15.0f);
	int ammo = cfg.getInt(path + "ammo", 1);
	float reloadTime = cfg.getFloat(path + "reloadTime", 7.0f);

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

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{0.0f, static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
}

void weapon::emplaceWeaponSniper(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.bullet.sniper.";

	float baseSpeed = cfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = cfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(cfg);

	float radius = cfg.getFloat(path + "radius", 0.2f);
	float hp = cfg.getFloat(path + "hp", 50.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 2.5f);
	float spreadMultiplier = cfg.getFloat(path + "spreadMultiplier", 0.01f);
	int bulletCount = cfg.getInt(path + "bulletCount", 1);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 6.0f);
	float lifespan = cfg.getFloat(path + "lifespan", 10.0f);
	float cooldown = cfg.getFloat(path + "cooldown", 1.5f);

	t_model_id model = context.modelManager.createSphere();

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

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
}

void weapon::emplaceWeaponBurstSniper(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.bullet.burstSniper.";

	float baseSpeed = cfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = cfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(cfg);

	float radius = cfg.getFloat(path + "radius", 0.2f);
	float hp = cfg.getFloat(path + "hp", 50.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 2.5f);
	float spreadMultiplier = cfg.getFloat(path + "spreadMultiplier", 5.0f);
	int bulletCount = cfg.getInt(path + "bulletCount", 1);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 6.0f);
	float lifespan = cfg.getFloat(path + "lifespan", 10.0f);
	float cooldown = cfg.getFloat(path + "cooldown", 0.35f);
	int ammo = cfg.getInt(path + "ammo", 20);
	float reloadTime = cfg.getFloat(path + "reloadTime", 12.0f);

	t_model_id model = context.modelManager.createSphere();

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

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
}

void weapon::emplaceWeaponBasic(GameContext &context, entt::entity entity)
{
	const auto& cfg = context.config;
	const std::string path = "weapons.bullet.basic.";

	float baseSpeed = cfg.getFloat("weapons.bullet.baseSpeed", 800.0f);
	float baseDamage = cfg.getFloat("weapons.bullet.baseDamage", 25.0f);
	float baseSpread = getBaseSpread(cfg);

	float radius = cfg.getFloat(path + "radius", 0.075f);
	float hp = cfg.getFloat(path + "hp", 1.0f);
	float damageMultiplier = cfg.getFloat(path + "damageMultiplier", 1.0f);
	float spreadMultiplier = cfg.getFloat(path + "spreadMultiplier", 5.0f);
	int bulletCount = cfg.getInt(path + "bulletCount", 1);
	float speedMultiplier = cfg.getFloat(path + "speedMultiplier", 1.2f);
	float lifespan = cfg.getFloat(path + "lifespan", 10.0f);
	float cooldown = cfg.getFloat(path + "cooldown", 0.25f);
	int ammo = cfg.getInt(path + "ammo", 30);
	float reloadTime = cfg.getFloat(path + "reloadTime", 2.0f);

	t_model_id model = context.modelManager.createSphere();

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

	emplaceBulletWeaponCommon(context, entity);
	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	context.registry.emplace_or_replace<Ammo>(entity, Ammo{static_cast<float>(ammo), static_cast<float>(ammo)});
	context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
}


