#include "shoot_3d.hpp"

entt::entity spawn_player(entt::registry& registry) {
    entt::entity player = registry.create();
    registry.emplace<Position>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Velocity>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(player, 1.0f, BLUE);
    registry.emplace<HP>(player, 1000, 1000);
    registry.emplace<Damage>(player, 50);
    registry.emplace<Player>(player, PLAYER_SPEED, SHOOT_COOLDOWN, 0.0f);
    return player;
}