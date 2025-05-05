#include "shoot_3d.hpp"

entt::entity spawnPlayer(entt::registry& registry) {
    entt::entity player = registry.create();
    registry.emplace<Position>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Velocity>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(player, 1.0f, BLUE);
    registry.emplace<HP>(player, 300.0f, 300.0f);
    registry.emplace<HPRegen>(player, 10.0f);
    registry.emplace<Damage>(player, 50.0f);
    registry.emplace<MaxSpeed>(player, 25.0f);
    registry.emplace<Player>(player);
	emplaceWeaponMachine_gun(registry, player);
    registry.emplace<AimRotation>(player, Vector3{ 0, 0, 0 });
    registry.emplace<AimTarget>(player, entt::null);
    return player;
}
