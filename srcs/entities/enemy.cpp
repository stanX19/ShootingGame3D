#include "entities.hpp"

entt::entity spawnBaseEnemy(GameContext &context, const Vector3& pos) {
	entt::entity enemy = context.registry.create();
	
	t_model_id shipModel = context.meshManager.loadModel("assets/Models/spacechip1/model/Intergalactic_Spaceship-(Wavefront).obj");
    context.registry.emplace<Position>(enemy, pos);
    context.registry.emplace<Velocity>(enemy);
    context.registry.emplace<Rotation>(enemy);
    context.registry.emplace<CollisionBody>(enemy, 1.0f);
    context.registry.emplace<RenderBody>(enemy, RenderBody{
		shipModel, 0.4f, GREEN
	});
    context.registry.emplace<HP>(enemy, 200.0f);
    context.registry.emplace<Damage>(enemy, 5000.0f);
    context.registry.emplace<MaxSpeed>(enemy, 10.0f);
    context.registry.emplace<TurnSpeed>(enemy, 2.5f);
    context.registry.emplace<PlayerTargetable>(enemy);
	
    context.registry.emplace<tag::Enemy>(enemy);
    context.registry.emplace<tag::Shaded>(enemy);
    context.registry.emplace<tag::RotationSyncModel>(enemy);
	return enemy;
}

entt::entity spawnEnemy(GameContext &context, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(context, pos);

	emplaceWeaponBasic(context, enemy);
	return enemy;
}

entt::entity spawnEliteEnemy(GameContext &context, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(context, pos);

	float radius = 3.0f;
    context.registry.emplace_or_replace<CollisionBody>(enemy, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(enemy);
    renderBody.color = DARKGREEN;
	renderBody.scale = radius * 0.4;
    context.registry.emplace_or_replace<HP>(enemy, 300.0f);
    context.registry.emplace_or_replace<HPRegen>(enemy, 10.0f);
    context.registry.emplace_or_replace<MaxSpeed>(enemy, 5.0f);

	emplaceWeaponSniper(context, enemy);

    context.registry.emplace_or_replace<tag::EliteEnemy>(enemy);
	return enemy;
}

entt::entity spawnFastEliteEnemy(GameContext &context, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(context, pos);

	float radius = 2.0f;
    context.registry.emplace_or_replace<CollisionBody>(enemy, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(enemy);
    renderBody.color = LIME;
	renderBody.scale = radius * 0.4;
    context.registry.emplace_or_replace<HP>(enemy, 160.0f);
    context.registry.emplace_or_replace<HPRegen>(enemy, 1.0f);
    context.registry.emplace_or_replace<MaxSpeed>(enemy, 40.0f);
    context.registry.emplace_or_replace<TurnSpeed>(enemy, 3.5f);

	emplaceWeaponMachineGun(context, enemy);

    context.registry.emplace_or_replace<tag::EliteEnemy>(enemy);
	return enemy;
}

entt::entity spawnMothershipEnemy(GameContext &context, const Vector3& pos) {
    entt::entity enemy = spawnBaseEnemy(context, pos);

	float radius = 15.0f;
    context.registry.emplace_or_replace<CollisionBody>(enemy, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(enemy);
    renderBody.color = Color{191, 245, 66, 255};
	renderBody.scale = radius * 0.4;
    context.registry.emplace_or_replace<HP>(enemy, 1000.0f);
    context.registry.emplace_or_replace<HPRegen>(enemy, 50.0f);
    context.registry.emplace_or_replace<MaxSpeed>(enemy, 10.0f);
    context.registry.emplace_or_replace<TurnSpeed>(enemy, 1.0f);

	emplaceWeaponSniper(context, enemy);
	emplaceWeaponMachineGun(context, spawnLinkedTurret(context, renderBody.color, enemy, {+radius * 1.5f, +radius * 0.8f, -2}));
	emplaceWeaponMachineGun(context, spawnLinkedTurret(context, renderBody.color, enemy, {+radius * 1.5f, -radius * 0.8f, -2}));
	emplaceWeaponMachineGun(context, spawnLinkedTurret(context, renderBody.color, enemy, {-radius * 1.5f, +radius * 0.8f, -2}));
	emplaceWeaponMachineGun(context, spawnLinkedTurret(context, renderBody.color, enemy, {-radius * 1.5f, -radius * 0.8f, -2}));
    context.registry.emplace_or_replace<tag::EliteEnemy>(enemy);
	return enemy;
}