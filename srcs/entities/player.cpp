#include "entities.hpp"
#include "utils.hpp"

namespace {
	Vector3 left = {1, 0, 0};
	Vector3 up = {0, 1, 0};
	Vector3 front = {0, 0, 1};

	void addTurretAt(GameContext &context, Color color, entt::entity &player, Vector3 relpos) {
		entt::entity turret = spawnLinkedTurret(context, color, player, relpos);
		emplaceWeaponMachineGun(context, turret);
	}

	void addWeapons(GameContext &context, entt::entity &player, Color color) {
    	context.registry.emplace<AimTarget>(player);
		context.registry.emplace<tag::weapon::AIControlledAim>(player);
		context.registry.emplace<tag::weapon::PlayerControlledShoot>(player);
		addTurretAt(context, color, player, left * 3 + up * 0.5 + front * -1);
		addTurretAt(context, color, player, left * -3 + up * 0.5 + front * -1);
		addTurretAt(context, color, player, left * 3 + up * -0.5 + front * -1);
		addTurretAt(context, color, player, left * -3 + up * -0.5 + front * -1);
		emplaceWeaponSniper(context, player);
	}
}

entt::entity spawnPlayer(GameContext &context) {
    entt::entity player = context.registry.create();

	// t_model_id shipModel = context.meshManager.loadModel("assets/Models/spaceship2/Intergalactic_Spaceships_Version_2.gltf");
	t_model_id shipModel = context.meshManager.loadModel("assets/Models/spaceship_custom_100/Spaceship1.obj");
    context.registry.emplace<Position>(player, Vector3{ 0, 0, 0 });
    context.registry.emplace<Velocity>(player);
    context.registry.emplace<Rotation>(player);
    context.registry.emplace<CollisionBody>(player, 1.0f);
    context.registry.emplace<RenderBody>(player, RenderBody{shipModel, 1.0f, BLUE});
    context.registry.emplace<HP>(player, 600.0f);
    context.registry.emplace<HPRegen>(player, 20.0f);
    context.registry.emplace<Damage>(player, 5000.0f);
    context.registry.emplace<MaxSpeed>(player, 80.0f);
    context.registry.emplace<TurnSpeed>(player, 5.0f);
	
    context.registry.emplace<tag::Shaded>(player);
    context.registry.emplace<tag::Player>(player);
    context.registry.emplace<tag::RotationSyncModel>(player);
	
	addWeapons(context, player, SKYBLUE);
    return player;
}
