#include "entities.hpp"
#include "components/factions.hpp"
#include "components/unit_camera.hpp"
#include "utils.hpp"

#include <array>
#include <string>

namespace {

bool knownWeapon(const GameContext& context, const std::string& id) {
	return !id.empty()
		&& context.weaponRegistry.getAllWeaponsMap().find(id)
			!= context.weaponRegistry.getAllWeaponsMap().end();
}

std::string randomStandardWeapon(const GameContext& context) {
	return context.weaponRegistry.getRandomStandardWeaponId(
		GetRandomValue(0, 100000)
	);
}

std::string randomSpecialWeapon(const GameContext& context) {
	return context.weaponRegistry.getRandomSpecialWeaponId(
		GetRandomValue(0, 100000)
	);
}

unit::Loadout makePlayerLoadout(
	GameContext& context,
	std::size_t mountCount
) {
	const std::array<std::string, 4> configured{
		context.config.loadout.w1,
		context.config.loadout.w2,
		context.config.loadout.w3,
		context.config.loadout.w4
	};
	unit::Loadout loadout;
	loadout.turretWeapons.resize(mountCount);
	for (std::size_t index = 0; index < mountCount; ++index) {
		const std::string& configuredId = configured[index % configured.size()];
		loadout.turretWeapons[index] = knownWeapon(context, configuredId)
			? configuredId
			: randomStandardWeapon(context);
	}
	loadout.specialWeapon = knownWeapon(context, context.config.loadout.special)
		? context.config.loadout.special
		: randomSpecialWeapon(context);
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
	const auto& playerDefinition = context.config.units().get("player");
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
		unit::spawnConfiguredUnit(context, "player", params);
	context.registry.emplace<SpawnsTrailParticle>(
		spawned.entity,
		SpawnsTrailParticle{0.3f, 0.1f}
	);
	addPlayerControlTags(context, spawned.entity);
	context.currentPlayer = spawned.entity;
	return spawned.entity;
}
