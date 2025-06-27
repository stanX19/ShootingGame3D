#include "entities.hpp"

static entt::entity spawnBaseTurret(GameContext &context, Color color) {
    entt::entity turret = context.registry.create();

	// t_model_id shipModel = context.meshManager.loadModel("assets/Models/spaceship2/Intergalactic_Spaceships_Version_2.gltf");
	// t_model_id shipModel = context.meshManager.loadModel("assets/Models/spaceship_custom_100/Spaceship1.obj");
	t_model_id shipModel = context.meshManager.createBox();
    context.registry.emplace<Position>(turret);
    context.registry.emplace<Rotation>(turret);
    context.registry.emplace<CollisionBody>(turret, 0.5f);
    context.registry.emplace<RenderBody>(turret, RenderBody{shipModel, 0.5f, color});
    context.registry.emplace<HP>(turret, 150.0f);
    context.registry.emplace<HPRegen>(turret, 10.0f);
    // context.registry.emplace<TurnSpeed>(turret, 2.5f);
    context.registry.emplace<tag::Shaded>(turret);
    context.registry.emplace<tag::AimDirectionSyncModel>(turret);
    return turret;
}

static void linkWithParent(GameContext &context, entt::entity &turret, entt::entity &parent, Vector3 relpos) {
	context.registry.emplace_or_replace<PositionAnchor>(turret, PositionAnchor{parent, relpos});
	context.registry.emplace_or_replace<RotationAnchor>(turret, RotationAnchor{parent});
	context.registry.emplace_or_replace<WeaponParent>(turret, WeaponParent{parent});
	context.registry.emplace_or_replace<DeathAnchor>(turret, DeathAnchor{parent, 2});
}

entt::entity spawnUnlinkedAutoTurret(GameContext &context, Color color) {
    entt::entity turret = spawnBaseTurret(context, color);
    context.registry.emplace<tag::weapon::AIControlledAim>(turret);
	context.registry.emplace<tag::weapon::AIControlledShoot>(turret);
    return turret;
}

entt::entity spawnLinkedTurret(GameContext &context, Color color, entt::entity &parent, Vector3 relpos) {
	entt::entity turret = spawnBaseTurret(context, color);
	linkWithParent(context, turret, parent, relpos);
	context.registry.emplace<tag::weapon::ParentControlledAim>(turret);
	context.registry.emplace<tag::weapon::ParentControlledShoot>(turret);
	
	return turret;
}

entt::entity spawnLinkedAutoTurret(GameContext &context, Color color, entt::entity &parent, Vector3 relpos) {
	entt::entity turret = spawnBaseTurret(context, color);
	linkWithParent(context, turret, parent, relpos);
	context.registry.emplace<tag::weapon::AIControlledAim>(turret);
	context.registry.emplace<tag::weapon::AIControlledShoot>(turret);

	return turret;
}