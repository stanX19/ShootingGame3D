#include "shoot_3d.hpp"

entt::entity spawn_player(entt::registry& registry) {
    entt::entity player = registry.create();
    registry.emplace<Position>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Velocity>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(player, 1.0f, BLUE);
    registry.emplace<HP>(player, 100.0f, 100.0f);
    registry.emplace<HPRegen>(player, 10.0f);
    registry.emplace<Damage>(player, 50.0f);
    registry.emplace<Player>(player, PLAYER_SPEED, 0.25f, 0.0f);
    return player;
}