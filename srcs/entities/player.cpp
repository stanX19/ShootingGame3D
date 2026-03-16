#include "entities.hpp"
#include "weapons.hpp"
#include "weapon_registry.hpp"
#include "utils.hpp"
#include "components/factions.hpp"
#include "components/unit_camera.hpp"
#include "components/sound.hpp"

namespace {
	Vector3 left = {1, 0, 0};
	Vector3 up = {0, 1, 0};
	Vector3 front = {0, 0, 1};

	void addWeapons(GameContext &context, entt::entity &player, Color color) {
		context.registry.emplace<AimTarget>(player);
		context.registry.emplace<tag::weapon::AIControlledAim>(player);
		context.registry.emplace<tag::weapon::PlayerControlledFire>(player);

		auto hookWeapon = [&](const std::string& id, Vector3 posOffset) {
			entt::entity turret = spawnLinkedTurret(context, color, player, posOffset);
			if (id.empty() || context.weaponRegistry.getAllWeaponsMap().find(id) == context.weaponRegistry.getAllWeaponsMap().end()) {
				context.weaponRegistry.emplaceRandomWeapon(context, turret);
			} else {
				context.weaponRegistry.emplaceWeaponById(context, turret, id);
			}
		};

		hookWeapon(context.config.loadout.w1, left * 3 + up * 0.5f + front * -1);
		hookWeapon(context.config.loadout.w2, left * -3 + up * 0.5f + front * -1);
		hookWeapon(context.config.loadout.w3, left * 4 + up * 0.5f + front * -2);
		hookWeapon(context.config.loadout.w4, left * -4 + up * 0.5f + front * -2);
	}

	void addSpecialWeapons(GameContext &context, entt::entity &player, [[maybe_unused]] Color color) noexcept {
		std::string id = context.config.loadout.special;
		if (id.empty() || context.weaponRegistry.getAllWeaponsMap().find(id) == context.weaponRegistry.getAllWeaponsMap().end()) {
			context.weaponRegistry.emplaceRandomSpecialWeapon(context, player);
		} else {
			context.weaponRegistry.emplaceWeaponById(context, player, id);
		}
		context.registry.emplace<tag::weapon::IsSpecialWeapon>(player);
	}
}

entt::entity spawnPlayer(GameContext &context) {
	return spawnPlayer(context, Vector3{0, 0, -context.config.ARENA_SIZE * 0.5f});
}

entt::entity spawnPlayer(GameContext &context, Vector3 pos) {
	entt::entity player = context.registry.create();

	const GameConfig& cfg = context.config;
	const float hp = cfg.getFloat("units.player.hp", 1500.0f);
	const float hpRegen = cfg.getFloat("units.player.hpRegen", 25.0f);
	const float shield = cfg.getFloat("units.player.shield", 3000.0f);
	const float shieldRegenDiv = cfg.getFloat("units.player.shieldRegenDivisor", 15.0f);
	const float speed = cfg.getFloat("units.player.speed", 80.0f);
	const float turnSpeed = cfg.getFloat("units.player.turnSpeed", 2.0f);
	const float damage = cfg.getFloat("units.player.damage", 500.0f);

	// t_model_id shipModel = context.modelManager.loadModel("assets/Models/spaceship2/Intergalactic_Spaceships_Version_2.gltf");
	t_model_id shipModel = context.modelManager.loadModel(cfg, "units.player.modelPath");
	// t_model_id shipModel = context.modelManager.loadModel("assets/Models/spaceship_custom_2/Spaceship2.glb");
	context.registry.emplace<Position>(player, pos);
	context.registry.emplace<Velocity>(player);
	context.registry.emplace<Rotation>(player, vector3ToRotation(Vector3{0, 0, 1}));
	context.registry.emplace<CollisionBody>(player, 1.0f);
	context.registry.emplace<RenderBody>(player, RenderBody{shipModel, BLUE, 1.0f});
	context.registry.emplace<HP>(player, hp);
	context.registry.emplace<HPRegen>(player, hpRegen);
	context.registry.emplace<EnergyShield>(player, shield);
	context.registry.emplace<EnergyShieldRegen>(player, shield / shieldRegenDiv);
	context.registry.emplace<Damage>(player, damage);
	context.registry.emplace<MaxSpeed>(player, speed);
	context.registry.emplace<TurnSpeed>(player, turnSpeed);
	context.registry.emplace<Score>(player);
	context.registry.emplace<camera::UnitCamera>(player);
	context.registry.emplace<Mass>(player, cfg.getFloat("units.player.mass", 1000.0f));

	context.registry.emplace<faction::Faction>(player, faction::FAC_BLUE);
	context.registry.emplace<tag::Targetable>(player);
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<tag::Shaded>(player);
	context.registry.emplace<tag::RotationSyncModel>(player);
	context.registry.emplace<tag::effect::DropDebris>(player);
	context.registry.emplace<tag::effect::ExplodeOnDeath>(player);
	context.registry.emplace<SpawnsTrailParticle>(player, SpawnsTrailParticle{0.3f, 0.1f});
	context.registry.emplace<sound::DeathSound>(player, sound::RANDOM_EXPLOSION, 0.5f);

	addWeapons(context, player, SKYBLUE);
	addSpecialWeapons(context, player, SKYBLUE);
	context.currentPlayer = player;
	return player;
}
