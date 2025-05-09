#include "shoot_3d.hpp"
#include "renderer.hpp"

static void spawnSunAndStars(entt::registry& registry) {
	entt::entity sun2 = registry.create();
	Position pos = {randomUnitVector3() * ARENA_SIZE * 14};
	float rad = GetRandomValue(ARENA_SIZE * 7, ARENA_SIZE * 10);
	registry.emplace<Position>(sun2, pos);
	registry.emplace<Body>(sun2, rad, Color{105, 205, 255, 255});
	// stars
	for (int i = 0; i < 100; i++) {
		entt::entity entity = registry.create();

		registry.emplace<Position>(entity, randomUnitVector3() * ARENA_SIZE * 10);
		registry.emplace<Body>(entity, GetRandomValue(10, 30) / 10.0f, WHITE);
	}
}

static void setup_camera(Camera3D& camera) {
    camera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

static void camaraFollowPlayer(entt::registry& registry, Camera3D &camera) {
	auto playerView = registry.view<Player, Position, Rotation>();
	for (auto entity : playerView) {
		Position& pos = playerView.get<Position>(entity);
		Rotation& rot = playerView.get<Rotation>(entity);
		Vector3 forward = GetForwardVector(rot);
		Vector3 up = GetUpVector(rot);
		
		// Vector3 cameraOffset = Vector3Add(Vector3Scale(forward, -10.0f), Vector3Scale(up, 5.0f));
		Vector3 cameraOffset = forward * -10 + up * 5;
		camera.position = pos.value + cameraOffset;
		camera.up = up;
		camera.target = pos.value + forward * 10.0f;
	}
}

static void resetGame(entt::registry& registry) {
    registry.clear();
    spawnPlayer(registry);
	spawnSunAndStars(registry);
}

int main() {
    InitWindow(1600, 900, "3D Space Shooter");
    SetTargetFPS(60);
    DisableCursor();

    Camera3D camera;
    setup_camera(camera);

    entt::registry registry;
    resetGame(registry);
	
    Renderer renderer(camera, registry);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- Update systems ---
        ecs_systems::playerMoveControl(registry, dt);
        ecs_systems::playerShootControl(registry);
        ecs_systems::playerAimTarget(registry);
        ecs_systems::enemyMoveControl(registry, dt);
        ecs_systems::enemyAimTarget(registry);
        ecs_systems::enemyMovement(registry, dt);
        ecs_systems::entityCollision(registry);
        ecs_systems::hpRegen(registry, dt);
        ecs_systems::entityLifetime(registry, dt);
        ecs_systems::hpCleanup(registry);
        ecs_systems::enemyRespawn(registry);
        ecs_systems::cleanOutOfBound(registry);
        ecs_systems::updatePlayerTargetable(registry);
        ecs_systems::asteroidRespawn(registry);
		ecs_systems::bulletTargetAim(registry);
        ecs_systems::ammoReload(registry, dt);
        ecs_systems::bulletWeaponShoot(registry, dt);

        camaraFollowPlayer(registry, camera);
        renderer.Render();

		if (IsKeyPressed(KEY_R)) {
			resetGame(registry);
		}		

        DrawFPS(10, 10);
    }
    CloseWindow();
    return 0;
}
