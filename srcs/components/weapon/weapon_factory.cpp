#include "weapons.hpp"
#include "classes/weapon_registry.hpp"
#include "utils/color_utils.hpp"
#include "utils/vector_rotation_utils.hpp"
#include "components/faction.hpp"
#include "components/sound.hpp"
#include "components/identity.hpp"
#include "components/render.hpp"
#include "components/lifetime.hpp"
#include "components/effect.hpp"
#include "components/combat.hpp"
#include "components/physics.hpp"
#include "components/spaceship.hpp"
#include "components/collision.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>

using namespace render;
using namespace combat;
using namespace collision;
using namespace lifetime;
using namespace physics;
using namespace spaceship;
using namespace identity;
using namespace effect;

namespace {

constexpr std::string_view DEFAULT_MODEL_PREFIX = "default:";

constexpr std::uint32_t hashString(std::string_view str) {
	std::uint32_t hash = 2166136261u;
	for (const char c : str) {
		hash = (hash ^ static_cast<std::uint8_t>(c)) * 16777619u;
	}
	return hash;
}

float getFloatDef(const nlohmann::json &j, const std::string &key, float defaultVal) {
	if (j.contains(key) && j[key].is_number()) {
		return j[key].get<float>();
	}
	return defaultVal;
}

int getIntDef(const nlohmann::json &j, const std::string &key, int defaultVal) {
	if (j.contains(key) && j[key].is_number()) {
		return j[key].get<int>();
	}
	return defaultVal;
}

bool getBoolDef(const nlohmann::json &j, const std::string &key, bool defaultVal) {
	if (j.contains(key) && j[key].is_boolean()) {
		return j[key].get<bool>();
	}
	return defaultVal;
}

std::string getStringDef(const nlohmann::json &j, const std::string &key, const std::string &defaultVal) {
	if (j.contains(key) && j[key].is_string()) {
		return j[key].get<std::string>();
	}
	return defaultVal;
}

Color getColorDef(const nlohmann::json &j, const std::string &key, Color defaultVal) {
	if (j.contains(key)) {
		return color_utils::parseColor(j[key], defaultVal);
	}
	return defaultVal;
}

Vector3 getVector3Def(const nlohmann::json &j, const std::string &key, Vector3 defaultVal) {
	if (j.contains(key) && j[key].is_array() && j[key].size() >= 3) {
		return Vector3{
			j[key][0].get<float>(),
			j[key][1].get<float>(),
			j[key][2].get<float>()
		};
	}
	return defaultVal;
}

void assembleTurretComponents(
	GameContext &context,
	entt::entity entity,
	const nlohmann::json &def
) {
	context.registry.emplace_or_replace<weapon::tag::IsWeapon>(entity);
	context.registry.emplace_or_replace<AimTarget>(entity);
	Vector3 initialAim = Vector3UnitZ;
	const auto *rot = context.registry.try_get<Rotation>(entity);
	if (rot != nullptr) {
		initialAim = getForwardVector(rot->value);
	}
	context.registry.emplace_or_replace<AimDirection>(entity, AimDirection{initialAim});

	const std::string soundPath = getStringDef(def, "sound", getStringDef(def, "shootSound", ""));
	const std::string soundType = getStringDef(def, "shootSoundType", "");
	sound::Id shootSoundId = sound::RANDOM_BULLET_SHOOT;
	if (soundType == "lazer") {
		shootSoundId = sound::RANDOM_LAZER_SHOOT;
	} else if (soundType == "missile") {
		shootSoundId = sound::RANDOM_MISSILE_SHOOT;
	}
	if (!soundPath.empty()) {
		shootSoundId = context.soundManager.loadSound(soundPath);
	}
	context.registry.emplace_or_replace<sound::ShootSound>(entity, shootSoundId, 0.5f);

	const float cooldown = getFloatDef(def, "cooldown", 0.0f);
	if (cooldown > 0.0f) {
		context.registry.emplace_or_replace<WeaponCooldown>(entity, WeaponCooldown{cooldown});
	}

	const int ammo = getIntDef(def, "ammo", 0);
	if (ammo > 0) {
		const float initialAmmo = getFloatDef(def, "initialAmmo", static_cast<float>(ammo));
		context.registry.emplace_or_replace<Ammo>(entity, Ammo{initialAmmo, static_cast<float>(ammo)});
		const float ammoRegen = getFloatDef(def, "ammoRegen", 0.0f);
		const float reloadTime = getFloatDef(def, "reloadTime", 0.0f);
		if (ammoRegen > 0.0f) {
			context.registry.emplace_or_replace<AmmoRegen>(entity, AmmoRegen{ammoRegen});
		} else if (reloadTime > 0.0f) {
			context.registry.emplace_or_replace<AmmoReload>(entity, AmmoReload{reloadTime});
		}
	}

	const float chargeTime = getFloatDef(def, "chargeTime", 0.0f);
	if (chargeTime > 0.0f) {
		const Color defaultChargeColor = ColorAlpha(getColorDef(def, "color", WHITE), 0.5f);
		const Color chargeColor = getColorDef(def, "chargeColor", defaultChargeColor);
		context.registry.emplace_or_replace<ChargedWeapon>(entity, ChargedWeapon{chargeTime, chargeColor});
	}

	const float extendFireReq = getFloatDef(def, "extendFireRequest", 0.0f);
	if (extendFireReq > 0.0f) {
		context.registry.emplace_or_replace<ExtendFireRequest>(entity, ExtendFireRequest{extendFireReq});
	}
	const float extendFireDur = getFloatDef(def, "extendFireDuration", 0.0f);
	if (extendFireDur > 0.0f) {
		context.registry.emplace_or_replace<ExtendFireDuration>(entity, ExtendFireDuration{extendFireDur});
	}

	context.registry.emplace_or_replace<WeaponName>(entity, getStringDef(def, "name", "Weapon"));
}

static t_model_id resolveDefaultPrimitive(ModelManager &modelManager, std::string_view key) {
	switch (hashString(key)) {
	case hashString("sphere"):
		return modelManager.createSphere(8, 8);
	case hashString("cube"):
	case hashString("box"):
		return modelManager.createCube(2.0f, 2.0f, 2.0f);
	case hashString("cylinder"):
		return modelManager.createCylinder(16, 1.0f, 2.0f);
	case hashString("plane"):
		return modelManager.createPlane(2.0f, 2.0f);
	default:
		return modelManager.createSphere(8, 8);
	}
}

t_model_id resolveProjectileModel(
	GameContext &context,
	const nlohmann::json &def
) {
	const std::string customPath = getStringDef(def, "modelPath", "");
	if (customPath.rfind(DEFAULT_MODEL_PREFIX.data(), 0) == 0) {
		const std::string_view key = std::string_view(customPath).substr(DEFAULT_MODEL_PREFIX.size());
		return resolveDefaultPrimitive(context.modelManager, key);
	}
	if (!customPath.empty()) {
		if (def.contains("modelCenterOffset") || def.contains("modelScale") || def.contains("modelRotationAxis")) {
			const Vector3 scale = getVector3Def(def, "modelScale", Vector3One());
			const Vector3 rotAxis = getVector3Def(def, "modelRotationAxis", Vector3UnitZ);
			const Vector3 center = getVector3Def(def, "modelCenterOffset", Vector3Zero());
			return context.modelManager.loadModel(customPath, scale, rotAxis, center);
		}
		return context.modelManager.loadModel(customPath);
	}
	return context.modelManager.createSphere(8, 8);
}

void assembleProjectileGuidance(
	GameContext &context,
	entt::entity bulletTemplate,
	const nlohmann::json &def,
	float turnSpeed
) {
	context.templateReg.emplace_or_replace<TurnSpeed>(bulletTemplate, TurnSpeed{turnSpeed});
	context.templateReg.emplace_or_replace<MoveTarget>(bulletTemplate);
	context.templateReg.emplace_or_replace<Rotation>(bulletTemplate);
	context.templateReg.emplace_or_replace<physics::tag::VelocitySyncRot>(bulletTemplate);
	context.templateReg.emplace_or_replace<spaceship::tag::AIMoveControl>(bulletTemplate);

	if (getBoolDef(def, "suicidal", false)) {
		context.templateReg.emplace_or_replace<spaceship::tag::Suicidal>(bulletTemplate);
	}
}

void assembleProjectileDeathEffects(
	GameContext &context,
	entt::entity bulletTemplate,
	const nlohmann::json &def,
	float radius
) {
	const float instantDamage = getFloatDef(def, "instantDamage", 0.0f);
	const float instantRadius = getFloatDef(def, "instantRadius", radius * 0.5f);
	if (instantDamage > 0.0f) {
		context.templateReg.emplace_or_replace<effect::InstantDamageOnDeath>(
			bulletTemplate,
			effect::InstantDamageOnDeath{instantDamage, instantRadius}
		);
	}

	const float explosionRadius = getFloatDef(def, "explosionFinalRadius", 0.0f);
	if (explosionRadius > 0.0f) {
		const float explosionStartRadius = getFloatDef(def, "explosionStartRadius", radius * 0.5f);
		const float explosionDuration = getFloatDef(def, "explosionDuration", effect::DEFAULT_EXPLOSION_DURATION);
		const float explosionDamage = getFloatDef(def, "explosionDamage", effect::DEFAULT_EXPLOSION_DAMAGE);
		const Color explosionColor = getColorDef(def, "explosionColor", effect::EXPLOSION_COLOR);
		context.templateReg.emplace_or_replace<effect::ExplodeOnDeath>(
			bulletTemplate,
			effect::ExplodeOnDeath{
				explosionRadius,
				explosionStartRadius,
				explosionDuration,
				explosionDamage,
				explosionColor
			}
		);
	}

	const float delayedDamage = getFloatDef(def, "delayedDamage", 0.0f);
	if (delayedDamage > 0.0f) {
		const float delayedTime = getFloatDef(def, "delayedDamageTime", 40.0f);
		context.templateReg.emplace_or_replace<combat::DelayedDamage>(bulletTemplate, combat::DelayedDamage{delayedDamage, delayedTime});
	}
}

void assembleProjectileTrail(
	GameContext &context,
	entt::entity bulletTemplate,
	const nlohmann::json &def
) {
	if (!getBoolDef(def, "trail", true)) {
		return;
	}
	const Color trailColor = getColorDef(def, "trailColor", GRAY);
	if (trailColor.a == 0) {
		return;
	}

	const float defaultWidth = getFloatDef(def, "radius", 0.05f) * 0.8f;
	const float trailWidth = getFloatDef(def, "trailWidth", std::max(0.15f, defaultWidth));
	const float trailMaxAge = getFloatDef(def, "trailMaxAge", 0.05f);
	const std::uint8_t trailMaxNodes = static_cast<std::uint8_t>(getIntDef(def, "trailMaxNodes", 2));
	const float trailMinDist = getFloatDef(def, "trailMinDistance", 1.0f);
	const float trailEndWidth = getFloatDef(def, "trailEndWidth", 0.0f);

	context.templateReg.emplace_or_replace<effect::HasSimpleTrail>(
		bulletTemplate,
		effect::HasSimpleTrail{trailWidth, trailColor, trailMaxAge, trailMaxNodes, trailMinDist, trailEndWidth}
	);
}

static void assembleProjectileTags(
	GameContext &context,
	entt::entity bullet,
	const nlohmann::json &def,
	float radius
) {
	if (getBoolDef(def, "isBullet", true)) {
		context.templateReg.emplace_or_replace<weapon::tag::Bullet>(bullet);
	}
	if (getBoolDef(def, "isMissile", false)) {
		context.templateReg.emplace_or_replace<weapon::tag::Missile>(bullet);
	}
	if (getBoolDef(def, "isKinetic", false)) {
		context.templateReg.emplace_or_replace<weapon::tag::Kinetic>(bullet);
	}
	if (getBoolDef(def, "isEnergy", false)) {
		context.templateReg.emplace_or_replace<weapon::tag::Energy>(bullet);
	}
	if (getBoolDef(def, "isLazer", false)) {
		context.templateReg.emplace_or_replace<weapon::tag::Lazer>(bullet);
	}
	if (getBoolDef(def, "syncModelRot", false)) {
		context.templateReg.emplace_or_replace<render::tag::VelocitySyncModelRot>(bullet);
	}
	if (getBoolDef(def, "deathSound", false)) {
		context.templateReg.emplace_or_replace<sound::DeathSound>(bullet, sound::RANDOM_EXPLOSION, std::min(1.0f, radius * 0.5f));
	}
}

static void assembleProjectileKinematics(
	GameContext &context,
	entt::entity bullet,
	const nlohmann::json &def,
	float radius
) {
	const float modelStretch = getFloatDef(def, "modelStretch", 0.0f);
	if (getBoolDef(def, "lazerStretch", false)) {
		context.templateReg.emplace_or_replace<ModelStrech>(bullet, ModelStrech{1.0f / (2.0f * radius)});
	} else if (modelStretch > 0.0f) {
		context.templateReg.emplace_or_replace<ModelStrech>(bullet, ModelStrech{modelStretch / (radius * 2.0f)});
	} else if (getBoolDef(def, "bulletStretch", false)) {
		context.templateReg.emplace_or_replace<ModelStrech>(bullet, ModelStrech{1.0f});
	}

	const float rotVel = getFloatDef(def, "rotationVelocity", 0.0f);
	if (std::abs(rotVel) > 1e-4f) {
		context.templateReg.emplace_or_replace<Rotation>(bullet);
		context.templateReg.emplace_or_replace<RotationVelocity>(bullet, QuaternionFromAxisAngle(Vector3UnitY, rotVel));
	}

	const float turnSpeed = getFloatDef(def, "turnSpeed", 0.0f);
	const bool isHoming = getBoolDef(def, "homing", false);
	if (turnSpeed > 0.0f || isHoming) {
		assembleProjectileGuidance(context, bullet, def, turnSpeed);
	}

	const float accel = getFloatDef(def, "acceleration", 0.0f);
	if (accel > 0.0f) {
		context.templateReg.emplace_or_replace<ScalarAcceleration>(bullet, ScalarAcceleration{accel});
	}

	if (getBoolDef(def, "targetable", false)) {
		context.templateReg.emplace_or_replace<combat::tag::Targetable>(bullet);
	}
}

entt::entity assembleProjectileTemplate(
	GameContext &context,
	const nlohmann::json &def
) {
	const auto &cfg = context.config;
	const float arenaBound = cfg.ARENA_SIZE + cfg.COMBAT_DIST * 2.0f;
	const Vector3 disappearBound = {arenaBound, arenaBound, arenaBound};

	const entt::entity bullet = context.templateReg.create();
	const float radius = getFloatDef(def, "radius", 0.05f);
	const float hp = getFloatDef(def, "hp", 1.0f);
	const float mass = getFloatDef(def, "mass", 0.0f);
	const float baseDmg = getFloatDef(def, "baseDamage", 25.0f);
	const float dmgMult = getFloatDef(def, "damageMultiplier", 1.0f);
	const float damage = getFloatDef(def, "damage", baseDmg * dmgMult);
	const float lifespan = getFloatDef(def, "lifespan", 10.0f) * getFloatDef(def, "lifespanMultiplier", 1.0f);
	const Color color = getColorDef(def, "color", WHITE);

	const t_model_id modelId = resolveProjectileModel(context, def);
	assembleProjectileTags(context, bullet, def, radius);

	context.templateReg.emplace_or_replace<HP>(bullet, HP{hp});
	context.templateReg.emplace_or_replace<Damage>(bullet, Damage{damage});
	context.templateReg.emplace_or_replace<Mass>(bullet, mass);
	context.templateReg.emplace_or_replace<CollisionBody>(bullet, CollisionBody{radius});
	context.templateReg.emplace_or_replace<RenderBody>(bullet, RenderBody{modelId, color, radius});
	const bool hasLifespan = (!def.contains("lifespan") || !def["lifespan"].is_null()) && (lifespan > 0.0f);
	if (hasLifespan) {
		context.templateReg.emplace_or_replace<Lifespan>(bullet, Lifespan{lifespan});
	}
	context.templateReg.emplace_or_replace<DisappearBound>(bullet, disappearBound * -1.0f, disappearBound);

	const std::string hitSoundPath = getStringDef(def, "hitSound", "");
	const std::string hitSoundType = getStringDef(def, "hitSoundType", "");
	sound::Id hitSoundId = (hitSoundType == "lazer") ? sound::RANDOM_LAZER_HIT : sound::RANDOM_BULLET_HIT;
	if (!hitSoundPath.empty()) {
		hitSoundId = context.soundManager.loadSound(hitSoundPath);
	}
	context.templateReg.emplace_or_replace<sound::HitSound>(bullet, hitSoundId, 0.4f);

	assembleProjectileKinematics(context, bullet, def, radius);
	assembleProjectileDeathEffects(context, bullet, def, radius);
	assembleProjectileTrail(context, bullet, def);

	return bullet;
}

} // namespace

void weapon::emplaceConfiguredWeapon(
	GameContext &context,
	entt::entity entity,
	const nlohmann::json &def
) {
	const entt::entity bulletTemplate = assembleProjectileTemplate(context, def);

	const float baseSpeed = getFloatDef(def, "baseSpeed", 1000.0f);
	const float speedMult = getFloatDef(def, "speedMultiplier", 1.0f);
	const float combatDist = context.config.COMBAT_DIST;
	const float rangeMult = getFloatDef(def, "effectiveRangeMultiplier", 2.0f);
	const float baseSpread = std::atan2(1.0f, combatDist * rangeMult);
	const float spreadMult = getFloatDef(def, "spreadMultiplier", 1.0f);
	const float spreadAngle = getFloatDef(def, "spreadAngle", baseSpread * spreadMult);

	Weapon weapon{bulletTemplate};
	weapon.bulletData.bulletCount = getIntDef(def, "bulletCount", 1);
	weapon.bulletData.speed = baseSpeed * speedMult;
	weapon.bulletData.spreadSin = std::sin(spreadAngle);

	context.registry.emplace_or_replace<Weapon>(entity, weapon);
	assembleTurretComponents(context, entity, def);
}
