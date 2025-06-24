#include "shoot_3d.hpp"

entt::entity spawnPlayer(GameContext &context) {
    entt::entity player = context.registry.create();
    context.registry.emplace<Position>(player, Vector3{ 0, 0, 0 });
    context.registry.emplace<Velocity>(player);
    context.registry.emplace<Rotation>(player);
    context.registry.emplace<CollisionBody>(player, 1.0f);
    context.registry.emplace<RenderBody>(player, 1.0f, BLUE);
    context.registry.emplace<HP>(player, 600.0f);
    context.registry.emplace<HPRegen>(player, 20.0f);
    context.registry.emplace<Damage>(player, 5000.0f);
    context.registry.emplace<MaxSpeed>(player, 40.0f);
    context.registry.emplace<TurnSpeed>(player, 2.5f);
	emplaceWeaponMachineGun(context, player);
    context.registry.emplace<AimTarget>(player);
    context.registry.emplace<AimDirection>(player);
	
    context.registry.emplace<tag::Shaded>(player);
    context.registry.emplace<tag::Player>(player);
    return player;
}
