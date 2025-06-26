#include "entities.hpp"

entt::entity spawnTurret(GameContext &context, Color color) {
    entt::entity turret = context.registry.create();

	// t_model_id shipModel = context.meshManager.loadModel("assets/Models/spaceship2/Intergalactic_Spaceships_Version_2.gltf");
	t_model_id shipModel = context.meshManager.loadModel("assets/Models/spaceship_custom_100/Spaceship1.obj");
    context.registry.emplace<Position>(turret);
    context.registry.emplace<Rotation>(turret);
    context.registry.emplace<CollisionBody>(turret, 1.0f);
    context.registry.emplace<RenderBody>(turret, RenderBody{shipModel, 1.0f, color});
    context.registry.emplace<HP>(turret, 150.0f);
    context.registry.emplace<HPRegen>(turret, 10.0f);
    // context.registry.emplace<Damage>(turret, 5000.0f);
    // context.registry.emplace<MaxSpeed>(turret, 40.0f);
    // context.registry.emplace<TurnSpeed>(turret, 2.5f);
	emplaceWeaponMachineGun(context, turret);
	
    context.registry.emplace<tag::Shaded>(turret);
    context.registry.emplace<tag::RotationSyncModel>(turret);
    return turret;
}
