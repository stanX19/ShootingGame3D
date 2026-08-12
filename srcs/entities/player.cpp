#include "entities.hpp"
#include "entities/spaceship_factory.hpp"
#include "weapons.hpp"
#include "weapon_registry.hpp"
#include "utils.hpp"
#include "components/factions.hpp"
#include "components/unit_camera.hpp"
#include "components/sound.hpp"

#include <array>
#include <stdexcept>

namespace {
	void addWeaponsToTurrets(
		GameContext& context,
		const spaceship::factory::SpawnedSpaceship& assembly
	) {
		entt::entity player = assembly.entity;
		context.registry.emplace<AimTarget>(player);
		context.registry.emplace<tag::weapon::AIControlledAim>(player);
		context.registry.emplace<tag::weapon::PlayerControlledFire>(player);

		const std::array<std::string, 4> weaponIds{
			context.config.loadout.w1,
			context.config.loadout.w2,
			context.config.loadout.w3,
			context.config.loadout.w4
		};
		if (assembly.turrets.size() != weaponIds.size())
			throw std::runtime_error(
				"Player spaceship must define exactly four mounts"
			);

		for (std::size_t index = 0; index < weaponIds.size(); ++index) {
			entt::entity turret = assembly.turret(index);
			const std::string& weaponId = weaponIds[index];
			if (weaponId.empty()
				|| context.weaponRegistry.getAllWeaponsMap().find(weaponId)
					== context.weaponRegistry.getAllWeaponsMap().end()) {
				context.weaponRegistry.emplaceRandomWeapon(context, turret);
				continue;
			}
			context.weaponRegistry.emplaceWeaponById(context, turret, weaponId);
		}
	}

	void addSpecialWeapons(
		GameContext& context,
		entt::entity& player,
		[[maybe_unused]] Color color
	) noexcept {
		const std::string& id = context.config.loadout.special;
		if (id.empty()
			|| context.weaponRegistry.getAllWeaponsMap().find(id)
				== context.weaponRegistry.getAllWeaponsMap().end()) {
			context.weaponRegistry.emplaceRandomSpecialWeapon(context, player);
		} else {
			context.weaponRegistry.emplaceWeaponById(context, player, id);
		}
		context.registry.emplace<tag::weapon::IsSpecialWeapon>(player);
	}
}

entt::entity spawnPlayer(GameContext& context) {
	return spawnPlayer(
		context,
		Vector3{0, 0, -context.config.ARENA_SIZE * 0.5f}
	);
}

entt::entity spawnPlayer(GameContext& context, Vector3 pos) {
	const GameConfig& cfg = context.config;
	const float hp = cfg.getFloat("units.player.hp", 1500.0f);
	const float hpRegen = cfg.getFloat("units.player.hpRegen", 25.0f);
	const float shield = cfg.getFloat("units.player.shield", 3000.0f);
	const float shieldRegenDiv =
		cfg.getFloat("units.player.shieldRegenDivisor", 15.0f);
	const float speed = cfg.getFloat("units.player.speed", 80.0f);
	const float turnSpeed = cfg.getFloat("units.player.turnSpeed", 2.0f);
	const float damage = cfg.getFloat("units.player.damage", 500.0f);

	spaceship::factory::SpawnParams params;
	params.position = pos;
	params.radius = cfg.getFloat("units.player.radius", 1.0f);
	params.bodyColor = WHITE;
	params.turretColor = SKYBLUE;
	params.rotation = vector3ToRotation(Vector3{0, 0, 1});
	params.faction = faction::FAC_BLUE;
	const auto assembly = spaceship::factory::spawnConfiguredSpaceship(context, "player", params);
	entt::entity player = assembly.entity;

	context.registry.emplace<HP>(player, hp);
	context.registry.emplace<HPRegen>(player, hpRegen);
	context.registry.emplace<EnergyShield>(player, shield);
	context.registry.emplace<EnergyShieldRegen>(player, shield / shieldRegenDiv);
	context.registry.emplace<Damage>(player, damage);
	context.registry.emplace<MaxSpeed>(player, speed);
	context.registry.emplace<TurnSpeed>(player, turnSpeed);
	context.registry.emplace<Score>(player);
	context.registry.emplace<camera::UnitCamera>(player);
	context.registry.emplace<Mass>(
		player,
		cfg.getFloat("units.player.mass", 1000.0f)
	);
	context.registry.emplace<effect::ExplodeOnDeath>(
		player,
		effect::ExplodeOnDeath::createFromRadDmg(1.0f, damage)
	);
	context.registry.emplace<SpawnsTrailParticle>(player, SpawnsTrailParticle{0.3f, 0.1f});
	context.registry.emplace<sound::DeathSound>(
		player,
		sound::RANDOM_EXPLOSION,
		0.5f
	);

	addWeaponsToTurrets(context, assembly);
	addSpecialWeapons(context, player, SKYBLUE);
	context.currentPlayer = player;
	return player;
}
