#include "entities.hpp"
#include "constants.hpp"
#include "weapons.hpp"
#include "utils.hpp"
#include "components/factions.hpp"

namespace {
	Vector3 left = {1, 0, 0};
	Vector3 up = {0, 1, 0};
	Vector3 front = {0, 0, 1};

	// void addTurretAt(GameContext &context, Color color, entt::entity &player, Vector3 relpos) {
	// 	entt::entity turret = spawnLinkedTurret(context, color, player, relpos);
	// 	emplaceWeaponMachineGun(context, turret);
	// }
	const float SHIELD = 3000.0f;
	const float SHIELD_REGEN = SHIELD / 15;

	void addWeapons(GameContext &context, entt::entity &player, Color color) {
		context.registry.emplace<AimTarget>(player);
		context.registry.emplace<tag::weapon::AIControlledAim>(player);
		context.registry.emplace<tag::weapon::PlayerControlledFire>(player);
		static int subWeapons = -1;
		++subWeapons;
		weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, color, player, left * 3 + up * 0.5 + front * -1), subWeapons);
		weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, color, player, left * -3 + up * 0.5 + front * -1), subWeapons);
		// weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, color, player, left * 3 + up * -0.5 + front * -1), subWeapons);
		// weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, color, player, left * -3 + up * -0.5 + front * -1), subWeapons);
		weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, color, player, left * 4 + up * 0.5 + front * -2), subWeapons);
		weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, color, player, left * -4 + up * 0.5 + front * -2), subWeapons);
		// weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, color, player, left * 4 + up * -0.5 + front * -2), subWeapons);
		// weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, color, player, left * -4 + up * -0.5 + front * -2), subWeapons);
		weapon::emplaceWeaponMissileBasic(context, player);
	}
}

entt::entity spawnPlayer(GameContext &context) {
	return spawnPlayer(context, Vector3{0, 0, -ARENA_SIZE * 0.5f});
}

entt::entity spawnPlayer(GameContext &context, Vector3 pos) {
	entt::entity player = context.registry.create();

	// t_model_id shipModel = context.modelManager.loadModel("assets/Models/spaceship2/Intergalactic_Spaceships_Version_2.gltf");
	t_model_id shipModel = context.modelManager.loadModel("assets/Models/spaceship_custom_100/Spaceship1.obj");
	// t_model_id shipModel = context.modelManager.loadModel("assets/Models/spaceship_custom_2/Spaceship2.glb");
	context.registry.emplace<Position>(player, pos);
	context.registry.emplace<Velocity>(player);
	context.registry.emplace<Rotation>(player, QuaternionFromAxisAngle(Vector3UnitY, 0.0f));
	context.registry.emplace<CollisionBody>(player, 1.0f);
	context.registry.emplace<RenderBody>(player, RenderBody{shipModel, BLUE, 1.0f});
	context.registry.emplace<HP>(player, 1500.0f);
	context.registry.emplace<HPRegen>(player, 25.0f);
	context.registry.emplace<EnergyShield>(player, SHIELD);
	context.registry.emplace<EnergyShieldRegen>(player, SHIELD_REGEN);
	context.registry.emplace<Damage>(player, 500.0f);
	context.registry.emplace<MaxSpeed>(player, 80.0f);
	context.registry.emplace<TurnSpeed>(player, 2.0f);
	context.registry.emplace<Score>(player);
	
	context.registry.emplace<faction::Faction>(player, faction::FAC_BLUE);
	context.registry.emplace<tag::Targetable>(player);
	context.registry.emplace<tag::Shaded>(player);
	context.registry.emplace<tag::RotationSyncModel>(player);
	context.registry.emplace<tag::effect::DropDebris>(player);
	context.registry.emplace<tag::effect::ExplodeOnDeath>(player);
	
	addWeapons(context, player, SKYBLUE);
	context.currentPlayer = player;
	return player;
}
