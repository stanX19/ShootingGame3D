#include "entities.hpp"
#include "weapons.hpp"
#include "weapon_registry.hpp"
#include "components/sound.hpp"
#include "components/factions.hpp"
#include "utils.hpp"

#include <algorithm>
#include <string_view>

namespace {
	spaceship::factory::SpawnedSpaceship spawnBaseUnit(
		GameContext& context,
		const Vector3& pos,
		float radius,
		faction::Faction faction,
		std::string_view shipId,
		turret::TurretControlMode turretControl
	) {
		spaceship::factory::SpawnParams params;
		params.position = pos;
		params.radius = radius;
		params.bodyColor = WHITE;
		params.faction = faction.value;
		params.turretControl = turretControl;
		
		const auto assembly = spaceship::factory::spawnConfiguredSpaceship(context, shipId, params);
		const entt::entity entity = assembly.entity;

		const GameConfig& cfg = context.config;
		const float baseHp = cfg.getFloat("units.base.hp", 1000.0f);
		const float baseShield = cfg.getFloat("units.base.shield", 500.0f);
		const float shieldRegenDiv =
			cfg.getFloat("units.base.shieldRegenDivisor", 15.0f);
		const float baseSpeed = cfg.getFloat("units.base.speed", 80.0f);
		const float baseTurn = cfg.getFloat("units.base.turnSpeed", 1.5f);
		const float baseDamage = cfg.getFloat("units.base.damage", 500.0f);
		const int baseScore = cfg.getInt("units.base.score", 500);

		context.registry.emplace<HP>(entity, baseHp);
		context.registry.emplace<EnergyShield>(entity, baseShield);
		context.registry.emplace<EnergyShieldRegen>(
			entity,
			baseShield / shieldRegenDiv
		);
		context.registry.emplace<Damage>(entity, baseDamage);
		context.registry.emplace<MaxSpeed>(entity, baseSpeed);
		context.registry.emplace<TurnSpeed>(entity, baseTurn);
		context.registry.emplace<Score>(entity);
		context.registry.emplace<KilledScore>(entity, baseScore);
		context.registry.emplace<MoveTarget>(entity);
		context.registry.emplace<Mass>(
			entity,
			cfg.getFloat("units.base.mass", 1000.0f)
		);
		context.registry.emplace<effect::ExplodeOnDeath>(
			entity,
			effect::ExplodeOnDeath::createFromRadDmg(radius, baseDamage)
		);
		context.registry.emplace_or_replace<sound::DeathSound>(
			entity,
			sound::RANDOM_EXPLOSION,
			0.5f * radius
		);
		context.registry.emplace<tag::weapon::AIControlledAim>(entity);
		context.registry.emplace<tag::weapon::AIControlledFire>(entity);
		context.registry.emplace<tag::weapon::IsSpecialWeapon>(entity);
		context.registry.emplace<tag::AIMoveControl>(entity);
		return assembly;
	}
}

entt::entity spawnUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const float radius = context.config.getFloat("units.basic.radius", 1.0f);
	const auto assembly = spawnBaseUnit(
		context,
		pos,
		radius,
		faction,
		"basic",
		turret::TurretControlMode::FollowParent
	);
	const entt::entity entity = assembly.entity;
	weapon::emplaceWeaponMissileBasic(
		context,
		entity,
		context.config.getSubConfig("weapons.missile.weapons.basic")
	);
	const int subWeapons = GetRandomValue(0, 1000);
	for (const entt::entity turretEntity : assembly.turrets)
		context.weaponRegistry.emplaceRandomWeapon(
			context,
			turretEntity,
			subWeapons
		);
	return entity;
}

entt::entity spawnEliteUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const GameConfig& cfg = context.config;
	const float radius = cfg.getFloat("units.elite.radius", 3.0f);
	const auto assembly = spawnBaseUnit(
		context,
		pos,
		radius,
		faction,
		"elite",
		turret::TurretControlMode::FollowParent
	);
	const entt::entity entity = assembly.entity;
	const float baseSpeed = cfg.getFloat("units.base.speed", 80.0f);
	const int baseScore = cfg.getInt("units.base.score", 500);

	context.registry.emplace_or_replace<HP>(
		entity,
		cfg.getFloat("units.elite.hp", 1200.0f)
	);
	context.registry.emplace_or_replace<HPRegen>(
		entity,
		cfg.getFloat("units.elite.hpRegen", 10.0f)
	);
	context.registry.emplace_or_replace<EnergyShield>(
		entity,
		cfg.getFloat("units.elite.shield", 1000.0f)
	);
	context.registry.emplace_or_replace<EnergyShieldRegen>(
		entity,
		cfg.getFloat("units.elite.shieldRegen", 25.0f)
	);
	context.registry.emplace_or_replace<MaxSpeed>(
		entity,
		baseSpeed * cfg.getFloat("units.elite.speedMultiplier", 0.5f)
	);
	context.registry.emplace_or_replace<KilledScore>(
		entity,
		baseScore * cfg.getInt("units.elite.scoreMultiplier", 2)
	);
	context.registry.emplace_or_replace<Mass>(
		entity,
		cfg.getFloat("units.elite.mass", 1000.0f)
	);
	context.weaponRegistry.emplaceRandomSpecialWeapon(context, entity);
	const int subWeapons = GetRandomValue(0, 1000);
	for (const entt::entity turretEntity : assembly.turrets)
		context.weaponRegistry.emplaceRandomWeapon(
			context,
			turretEntity,
			subWeapons
		);
	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}

entt::entity spawnFastEliteUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const GameConfig& cfg = context.config;
	const float radius = cfg.getFloat("units.fastElite.radius", 2.0f);
	const auto assembly = spawnBaseUnit(
		context,
		pos,
		radius,
		faction,
		"fastElite",
		turret::TurretControlMode::FollowParent
	);
	const entt::entity entity = assembly.entity;
	const float baseSpeed = cfg.getFloat("units.base.speed", 80.0f);
	const int baseScore = cfg.getInt("units.base.score", 500);

	context.registry.emplace_or_replace<HP>(
		entity,
		cfg.getFloat("units.fastElite.hp", 720.0f)
	);
	context.registry.emplace_or_replace<HPRegen>(
		entity,
		cfg.getFloat("units.fastElite.hpRegen", 1.0f)
	);
	context.registry.emplace_or_replace<MaxSpeed>(
		entity,
		baseSpeed * cfg.getFloat("units.fastElite.speedMultiplier", 2.0f)
	);
	context.registry.emplace_or_replace<KilledScore>(
		entity,
		baseScore * cfg.getInt("units.fastElite.scoreMultiplier", 2)
	);
	context.registry.emplace_or_replace<Mass>(
		entity,
		cfg.getFloat("units.fastElite.mass", 1000.0f)
	);
	context.weaponRegistry.emplaceRandomSpecialWeapon(context, entity);
	const int subWeapons = GetRandomValue(0, 1000);
	for (const entt::entity turretEntity : assembly.turrets)
		context.weaponRegistry.emplaceRandomWeapon(
			context,
			turretEntity,
			subWeapons
		);
	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}

entt::entity spawnTerminatorUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const float baseSpeed = context.config.getFloat("units.base.speed", 80.0f);
	const float baseTurn = context.config.getFloat("units.base.turnSpeed", 1.5f);
	const int baseScore = context.config.getInt("units.base.score", 500);
	const GameConfig& cfg = context.config.getSubConfig("units.terminator");
	const float radius = cfg.getFloat("radius", 2.0f);
	const auto assembly = spawnBaseUnit(
		context,
		pos,
		radius,
		faction,
		"terminator",
		turret::TurretControlMode::Autonomous
	);
	const entt::entity entity = assembly.entity;

	context.registry.emplace_or_replace<HP>(entity, cfg.getFloat("hp", 3200.0f));
	context.registry.emplace_or_replace<HPRegen>(
		entity,
		cfg.getFloat("hpRegen", 50.0f)
	);
	context.registry.emplace_or_replace<EnergyShield>(
		entity,
		cfg.getFloat("shield", 1500.0f)
	);
	context.registry.emplace_or_replace<EnergyShieldRegen>(
		entity,
		cfg.getFloat("shieldRegen", 100.0f)
	);
	context.registry.emplace_or_replace<MaxSpeed>(
		entity,
		baseSpeed * cfg.getFloat("speedMultiplier", 1.5f)
	);
	context.registry.emplace_or_replace<TurnSpeed>(
		entity,
		baseTurn * cfg.getFloat("turnSpeedMultiplier", 2.5f)
	);
	context.registry.emplace_or_replace<KilledScore>(
		entity,
		baseScore * cfg.getInt("scoreMultiplier", 50)
	);
	context.registry.emplace_or_replace<Mass>(
		entity,
		cfg.getFloat("mass", 10000.0f)
	);

	const int randomSeed = GetRandomValue(0, 1000);
	for (const entt::entity turretEntity : assembly.turrets)
		context.weaponRegistry.emplaceRandomWeapon(
			context,
			turretEntity,
			randomSeed
		);
	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}

entt::entity spawnMothershipUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const GameConfig& cfg = context.config;
	const float radius = cfg.getFloat("units.mothership.radius", 6.0f);
	const auto assembly = spawnBaseUnit(
		context,
		pos,
		radius,
		faction,
		"mothership",
		turret::TurretControlMode::FollowParent
	);
	const entt::entity entity = assembly.entity;
	const float baseSpeed = cfg.getFloat("units.base.speed", 80.0f);
	const float baseTurn = cfg.getFloat("units.base.turnSpeed", 1.5f);
	const int baseScore = cfg.getInt("units.base.score", 500);

	context.registry.emplace_or_replace<HP>(
		entity,
		cfg.getFloat("units.mothership.hp", 4800.0f)
	);
	context.registry.emplace_or_replace<HPRegen>(
		entity,
		cfg.getFloat("units.mothership.hpRegen", 50.0f)
	);
	context.registry.emplace_or_replace<EnergyShield>(
		entity,
		cfg.getFloat("units.mothership.shield", 1000.0f)
	);
	context.registry.emplace_or_replace<EnergyShieldRegen>(
		entity,
		cfg.getFloat("units.mothership.shieldRegen", 100.0f)
	);
	context.registry.emplace_or_replace<MaxSpeed>(
		entity,
		baseSpeed * cfg.getFloat("units.mothership.speedMultiplier", 0.25f)
	);
	context.registry.emplace_or_replace<TurnSpeed>(
		entity,
		baseTurn * cfg.getFloat("units.mothership.turnSpeedMultiplier", 0.5f)
	);
	context.registry.emplace_or_replace<KilledScore>(
		entity,
		baseScore * cfg.getInt("units.mothership.scoreMultiplier", 5)
	);
	context.registry.emplace_or_replace<Mass>(
		entity,
		cfg.getFloat("units.mothership.mass", 5000.0f)
	);

	weapon::emplaceWeaponMissileFlares(
		context,
		entity,
		context.config.getSubConfig("weapons.missile.weapons.flares")
	);
	const int randomSeed = GetRandomValue(0, 1000);
	const std::size_t randomTurretCount =
		std::min<std::size_t>(4, assembly.turrets.size());
	for (std::size_t index = 0; index < randomTurretCount; ++index)
		context.weaponRegistry.emplaceRandomWeapon(
			context,
			assembly.turret(index),
			randomSeed
		);
	for (std::size_t index = 4; index < assembly.turrets.size(); ++index)
		weapon::emplaceWeaponBasic(
			context,
			assembly.turret(index),
			context.config.getSubConfig("weapons.bullet.weapons.basic")
		);
	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}
