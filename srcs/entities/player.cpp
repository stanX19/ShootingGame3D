#include "entities.hpp"
#include "components/factions.hpp"
#include "components/unit_camera.hpp"
#include "utils.hpp"

#include <stdexcept>
#include <string>

namespace {

std::string selectedPlayerUnitId(const GameContext& context) {
	const std::string id = context.config.getString("loadout.shipId", "");
	if (id.empty())
		throw std::invalid_argument("PLAYER: loadout.shipId is required");
	if (!context.config.units().contains(id))
		throw std::invalid_argument(
			"PLAYER: loadout.shipId references an unknown unit: " + id
		);
	return id;
}

unit::Loadout makePlayerLoadout(
	GameContext& context,
	std::size_t mountCount
) {
	unit::Loadout loadout;
	loadout.turretWeapons.resize(mountCount, "bullet.basic");
	for (std::size_t index = 0; index < mountCount; ++index) {
		if (index < context.config.loadout.turretWeapons.size()
			&& !context.config.loadout.turretWeapons[index].empty())
			loadout.turretWeapons[index] =
				context.config.loadout.turretWeapons[index];
	}
	loadout.specialWeapon = context.config.loadout.specialWeapon.empty()
		? "missile.basic"
		: context.config.loadout.specialWeapon;
	return loadout;
}

void addPlayerControlTags(GameContext& context, entt::entity player) {
	// Weapon attachment may already install AimTarget on the player entity.
	// Keep the player-control adapter idempotent so the spawn order cannot
	// trigger EnTT's duplicate-component assertion.
	context.registry.emplace_or_replace<AimTarget>(player);
	context.registry.emplace<tag::weapon::AIControlledAim>(player);
	context.registry.emplace<tag::weapon::PlayerControlledFire>(player);
	context.registry.emplace<camera::UnitCamera>(player);
}

} // namespace

entt::entity spawnPlayer(GameContext& context) {
	return spawnPlayer(
		context,
		Vector3{0, 0, -context.config.ARENA_SIZE * 0.5f}
	);
}

entt::entity spawnPlayer(GameContext& context, Vector3 pos) {
	const std::string unitId = selectedPlayerUnitId(context);
	const auto& playerDefinition = context.config.units().get(unitId);
	const std::size_t mountCount = context.config.spaceship().get(
		playerDefinition.spaceshipReference
	).mounts.size();
	unit::SpawnParams params;
	params.position = pos;
	params.faction = faction::FAC_BLUE;
	params.bodyColor = WHITE;
	params.turretColor = SKYBLUE;
	params.rotation = vector3ToRotation(Vector3{0, 0, 1});
	params.loadout = makePlayerLoadout(context, mountCount);
	const unit::SpawnedUnit spawned =
		unit::spawnConfiguredUnit(context, unitId, params);
	if (context.registry.all_of<tag::EliteUnit>(spawned.entity))
		context.registry.remove<tag::EliteUnit>(spawned.entity);
	context.registry.emplace<SpawnsTrailParticle>(
		spawned.entity,
		SpawnsTrailParticle{0.3f, 0.1f}
	);
	addPlayerControlTags(context, spawned.entity);
	context.currentPlayer = spawned.entity;
	return spawned.entity;
}
