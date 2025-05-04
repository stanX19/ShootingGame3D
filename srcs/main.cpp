#include "shoot_3d.hpp"
#include "renderer.hpp"

void setup_camera(Camera3D& camera) {
    camera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

entt::entity create_player(entt::registry& registry) {
    entt::entity player = registry.create();
    registry.emplace<Position>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Velocity>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(player, 1.0f, BLUE);
    registry.emplace<HP>(player, 100, 100);
    registry.emplace<Damage>(player, 10);
    registry.emplace<Player>(player, PLAYER_SPEED, SHOOT_COOLDOWN, 0.0f);
    return player;
}

void spawn_initial_enemies(entt::registry& registry) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        float x = GetRandomValue(-ARENA_SIZE + 5, ARENA_SIZE - 5);
        float z = GetRandomValue(-ARENA_SIZE + 5, ARENA_SIZE - 5);
        spawn_enemy(registry, Vector3{ x, 0, z });
    }
}

void reset_game(entt::registry& registry) {
    registry.clear();
    create_player(registry);
    spawn_initial_enemies(registry);
}

int main() {
    InitWindow(1280, 720, "3D Space Shooter");
    SetTargetFPS(60);
    DisableCursor();

    Camera3D camera;
    setup_camera(camera);

    entt::registry registry;
    create_player(registry);
    spawn_initial_enemies(registry);

    Renderer renderer(camera, registry);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- Update systems ---
        ecs_system::player_update(registry, dt);
        ecs_system::enemy_update(registry, dt);
        ecs_system::entity_movement(registry, dt);
        ecs_system::entity_collision(registry);
        ecs_system::entity_lifetime(registry, dt);
        ecs_system::entity_cleanup(registry);
        ecs_system::enemy_respawn(registry);

        // --- Camera follow player ---
        auto playerView = registry.view<Player, Position, Rotation>();
        for (auto entity : playerView) {
            Position& pos = playerView.get<Position>(entity);
            Rotation& rot = playerView.get<Rotation>(entity);
            Vector3 forward = GetForwardVector(rot);
            camera.position = Vector3Add(pos.value, Vector3Scale(forward, -4.0f));
            camera.position.y += 3.0f;
            camera.target = Vector3Add(pos.value, Vector3Scale(forward, 10.0f));
        }

        // --- Render everything ---
        renderer.Render();

		if (IsKeyPressed(KEY_R)) {
			reset_game(registry);
		}		

        DrawFPS(10, 10);
    }
    CloseWindow();
    return 0;
}
