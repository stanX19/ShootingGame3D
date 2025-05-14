#include "shoot_3d.hpp"

entt::entity spawnPlayer(entt::registry& registry) {
    entt::entity player = registry.create();
    registry.emplace<Position>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Velocity>(player);
    registry.emplace<Rotation>(player);
    registry.emplace<CollisionBody>(player, 1.0f);
    registry.emplace<RenderBody>(player, 1.0f, BLUE);
    registry.emplace<HP>(player, 600.0f);
    registry.emplace<HPRegen>(player, 20.0f);
    registry.emplace<Damage>(player, 5000.0f);
    registry.emplace<MaxSpeed>(player, 40.0f);
    registry.emplace<TurnSpeed>(player, 2.5f);
	emplaceWeaponMachineGun(registry, player);
    registry.emplace<AimTarget>(player);
    registry.emplace<AimDirection>(player);
	
    registry.emplace<tag::Shaded>(player);
    registry.emplace<tag::Player>(player);
    return player;
}
