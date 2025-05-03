#include "shoot_3d.hpp"

void ecs_system::enemy_respawn(entt::registry& registry) {
    auto enemyView = registry.view<Enemy>();
    int enemiesToSpawn = ENEMY_COUNT - (int)enemyView.size(); // Cast to int

    for (int i = 0; i < enemiesToSpawn; i++) {
        float x = GetRandomValue(-ARENA_SIZE + 5, ARENA_SIZE - 5);
        float z = GetRandomValue(-ARENA_SIZE + 5, ARENA_SIZE - 5);
        spawn_enemy(registry, Vector3{ x, 0, z });
    }
}