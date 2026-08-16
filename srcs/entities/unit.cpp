#include "entities.hpp"
#include "entities/unit.hpp"
#include "components/factions.hpp"
#include "components/sound.hpp"
#include "weapon_registry.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

bool isKnownWeapon(const GameContext& context, std::string_view id) {
	return !id.empty()
		&& context.weaponRegistry.getAllWeaponsMap().find(std::string(id))
			!= context.weaponRegistry.getAllWeaponsMap().end();
}

std::size_t mountCount(
	const GameContext& context,
	std::string_view unitId
) {
	const auto& definition = context.config.units().get(unitId);
	return context.config.spaceship().get(
		definition.spaceshipReference
	).mounts.size();
}

std::vector<std::string> randomTurretLoadout(
	const GameContext& context,
	std::string_view unitId,
	int seed
) {
	return std::vector<std::string>(
		mountCount(context, unitId),
		context.weaponRegistry.getRandomStandardWeaponId(seed)
	);
}

std::string resolveWeaponId(
	const GameContext& context,
	std::string_view unitId,
	std::string_view slotType,
	std::size_t slotIndex,
	const std::string& requestedId,
	std::string_view fallbackId
) {
	if (requestedId.empty())
		return {};
	if (isKnownWeapon(context, requestedId))
		return requestedId;

	std::cerr << "UNIT: unknown " << slotType << " weapon '"
		<< requestedId << "' for " << unitId << "[" << slotIndex
		<< "]; using '" << fallbackId << "' instead\n";
	if (!isKnownWeapon(context, fallbackId)) {
		std::cerr << "UNIT: fallback weapon '" << fallbackId
			<< "' is unavailable; leaving " << unitId << "[" << slotIndex
			<< "] unarmed\n";
		return {};
	}
	return std::string(fallbackId);
}

void addEnemyControlTags(GameContext& context, entt::entity entity) {
	context.registry.emplace<MoveTarget>(entity);
	context.registry.emplace<tag::weapon::AIControlledAim>(entity);
	context.registry.emplace<tag::weapon::AIControlledFire>(entity);
	context.registry.emplace<tag::AIMoveControl>(entity);
}

unit::SpawnParams makeSpawnParams(
	const Vector3& position,
	faction::Faction faction,
	turret::TurretControlMode turretControl,
	unit::Loadout loadout
) {
	unit::SpawnParams params;
	params.position = position;
	params.faction = faction.value;
	params.bodyColor = WHITE;
	params.turretColor = WHITE;
	params.turretControl = turretControl;
	params.loadout = std::move(loadout);
	return params;
}

entt::entity spawnEnemyWithLoadout(
	GameContext& context,
	std::string_view unitId,
	const Vector3& position,
	faction::Faction faction,
	turret::TurretControlMode turretControl,
	unit::Loadout loadout
) {
	const unit::SpawnedUnit spawned = unit::spawnConfiguredUnit(
		context,
		unitId,
		makeSpawnParams(position, faction, turretControl, std::move(loadout))
	);
	addEnemyControlTags(context, spawned.entity);
	return spawned.entity;
}

} // namespace

namespace unit {

entt::entity SpawnedUnit::turret(std::size_t index) const {
	if (index >= turrets.size())
		throw std::out_of_range("UNIT: turret index out of range");
	return turrets[index];
}

SpawnedUnit spawnConfiguredUnit(
	GameContext& context,
	std::string_view unitId,
	const SpawnParams& params
) {
	const auto& definition = context.config.units().get(unitId);
	spaceship::factory::SpawnParams spaceshipParams;
	spaceshipParams.position = params.position;
	spaceshipParams.radius = definition.stats.collisionRadius;
	spaceshipParams.bodyColor = params.bodyColor;
	spaceshipParams.turretColor = params.turretColor;
	spaceshipParams.rotation = params.rotation;
	spaceshipParams.faction = params.faction;
	spaceshipParams.turretControl = params.turretControl;

	const auto assembly = spaceship::factory::spawnConfiguredSpaceship(
		context,
		definition.spaceshipReference,
		spaceshipParams
	);
	const entt::entity entity = assembly.entity;
	const auto& stats = definition.stats;
	const auto& effects = definition.effects;

	context.registry.emplace<HP>(entity, stats.hp);
	context.registry.emplace<HPRegen>(entity, stats.hpRegen);
	context.registry.emplace<EnergyShield>(entity, stats.shield);
	context.registry.emplace<EnergyShieldRegen>(entity, stats.shieldRegen);
	context.registry.emplace<Damage>(entity, stats.damage);
	context.registry.emplace<MaxSpeed>(entity, stats.maxSpeed);
	context.registry.emplace<TurnSpeed>(entity, stats.turnSpeed);
	context.registry.emplace<Mass>(entity, stats.mass);
	context.registry.emplace<Score>(entity, stats.score);
	context.registry.emplace<KilledScore>(entity, stats.killedScore);
	context.registry.emplace<effect::ExplodeOnDeath>(
		entity,
		effect::ExplodeOnDeath::createFromStartEndRad(
			stats.collisionRadius * 0.7f,
			stats.collisionRadius * 10.0f * effects.explosionRadiusScale,
			stats.damage
		)
	);
	context.registry.emplace<sound::DeathSound>(
		entity,
		sound::RANDOM_EXPLOSION,
		stats.collisionRadius * effects.deathSoundRadiusScale
	);
	if (definition.elite)
		context.registry.emplace<tag::EliteUnit>(entity);

	for (std::size_t index = 0;
		index < params.loadout.turretWeapons.size()
		&& index < assembly.turrets.size();
		++index) {
		const std::string weaponId = resolveWeaponId(
			context,
			unitId,
			"turret",
			index,
			params.loadout.turretWeapons[index],
			"bullet.basic"
		);
		if (weaponId.empty())
			continue;
		context.weaponRegistry.emplaceWeaponById(
			context,
			assembly.turret(index),
			weaponId
		);
	}
	const std::string specialWeaponId = resolveWeaponId(
		context,
		unitId,
		"special",
		0,
		params.loadout.specialWeapon,
		"missile.basic"
	);
	if (!specialWeaponId.empty()) {
		context.weaponRegistry.emplaceWeaponById(
			context,
			entity,
			specialWeaponId
		);
		context.registry.emplace<tag::weapon::IsSpecialWeapon>(entity);
	}

	return SpawnedUnit{assembly.entity, assembly.turrets};
}

} // namespace unit

entt::entity spawnUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	unit::Loadout loadout;
	loadout.turretWeapons = randomTurretLoadout(
		context,
		"basic",
		GetRandomValue(0, 1000)
	);
	loadout.specialWeapon = "missile.basic";
	return spawnEnemyWithLoadout(
		context,
		"basic",
		pos,
		faction,
		turret::TurretControlMode::FollowParent,
		std::move(loadout)
	);
}

entt::entity spawnEliteUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const int seed = GetRandomValue(0, 1000);
	unit::Loadout loadout;
	loadout.turretWeapons = randomTurretLoadout(context, "elite", seed);
	loadout.specialWeapon = context.weaponRegistry.getRandomSpecialWeaponId(seed);
	return spawnEnemyWithLoadout(
		context,
		"elite",
		pos,
		faction,
		turret::TurretControlMode::FollowParent,
		std::move(loadout)
	);
}

entt::entity spawnFastEliteUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const int seed = GetRandomValue(0, 1000);
	unit::Loadout loadout;
	loadout.turretWeapons = randomTurretLoadout(context, "fastElite", seed);
	loadout.specialWeapon = context.weaponRegistry.getRandomSpecialWeaponId(seed);
	return spawnEnemyWithLoadout(
		context,
		"fastElite",
		pos,
		faction,
		turret::TurretControlMode::FollowParent,
		std::move(loadout)
	);
}

entt::entity spawnTerminatorUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const int seed = GetRandomValue(0, 1000);
	unit::Loadout loadout;
	loadout.turretWeapons = randomTurretLoadout(context, "terminator", seed);
	return spawnEnemyWithLoadout(
		context,
		"terminator",
		pos,
		faction,
		turret::TurretControlMode::Autonomous,
		std::move(loadout)
	);
}

entt::entity spawnMothershipUnit(
	GameContext& context,
	const Vector3& pos,
	faction::Faction faction
) {
	const int seed = GetRandomValue(0, 1000);
	unit::Loadout loadout;
	loadout.turretWeapons = randomTurretLoadout(
		context,
		"mothership",
		seed
	);
	for (std::size_t index = 4; index < loadout.turretWeapons.size(); ++index)
		loadout.turretWeapons[index] = "bullet.basic";
	loadout.specialWeapon = "missile.flares";
	return spawnEnemyWithLoadout(
		context,
		"mothership",
		pos,
		faction,
		turret::TurretControlMode::FollowParent,
		std::move(loadout)
	);
}
