#include "entities.hpp"


static void addConnectedWeapons(GameContext &context, entt::entity &player, Color color) {
	entt::entity weapon1 = spawnTurret(context, color);
	context.registry.emplace_or_replace<PositionAnchor>(weapon1, PositionAnchor{player, {4, 0, 0}});
	context.registry.emplace_or_replace<RotationAnchor>(weapon1, RotationAnchor{player});
	entt::entity weapon2 = spawnTurret(context, color);
	context.registry.emplace_or_replace<PositionAnchor>(weapon2, PositionAnchor{player, {-4, 0, 0}});
	context.registry.emplace_or_replace<RotationAnchor>(weapon2, RotationAnchor{player});
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
    context.registry.emplace<MaxSpeed>(player, 40.0f);
    context.registry.emplace<TurnSpeed>(player, 2.5f);
	
    context.registry.emplace<tag::Shaded>(player);
    context.registry.emplace<tag::Player>(player);
    context.registry.emplace<tag::RotationSyncModel>(player);

	addConnectedWeapons(context, player, SKYBLUE);
    return player;
}
