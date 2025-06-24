#include "shoot_3d.hpp"
#include "renderer.hpp"

static void spawnSunAndStars(GameContext &context) {
	entt::entity sun2 = context.registry.create();
	Position pos = {randomUnitVector3() * ARENA_SIZE * 14};
	t_model_id sunModel = context.meshManager.createSphere(64, 64);
	float rad = GetRandomValue(ARENA_SIZE * 7, ARENA_SIZE * 10);

	context.registry.emplace<Position>(sun2, pos);
	context.registry.emplace<RenderBody>(sun2, sunModel, rad, Color{105, 205, 255, 255});
	context.registry.emplace<tag::LightSource>(sun2, rad, Color{105, 205, 255, 255});
	
	// stars
	t_model_id starsModel = context.meshManager.createSphere();

	for (int i = 0; i < 100; i++) {
		entt::entity entity = context.registry.create();

		context.registry.emplace<Position>(entity, randomUnitVector3() * ARENA_SIZE * 10);
		context.registry.emplace<RenderBody>(entity, starsModel, GetRandomValue(10, 30) * 20.0f / ARENA_SIZE, WHITE);
	}
}

static void setup_camera(Camera3D& camera) {
    camera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

static void camaraFollowPlayer(GameContext &context, Camera3D &camera) {
	auto playerView = context.registry.view<tag::Player, Position, Rotation>();
	for (auto entity : playerView) {
		Position& pos = playerView.get<Position>(entity);
		Rotation& rot = playerView.get<Rotation>(entity);
		Vector3 forward = GetForwardVector(rot);
		Vector3 up = GetUpVector(rot);

		// Vector3 cameraOffset = Vector3Add(Vector3Scale(forward, -10.0f), Vector3Scale(up, 5.0f));
		if (IsKeyDown(KEY_RIGHT_SHIFT)) {
			camera.position = pos.value + forward * 10 + up * 5;
			camera.up = up;
			camera.target = pos.value + forward * -10.0f;
		} else {
			camera.position = pos.value + forward * -10 + up * 5;
			camera.up = up;
			camera.target = pos.value + forward * 10.0f;
		}
	}
}

static void resetGame(GameContext &context) {
    context.registry.clear();
    spawnPlayer(context);
	spawnSunAndStars(context);
}

int main() {
    InitWindow(1600, 900, "3D Space Shooter");
    SetTargetFPS(60);
    DisableCursor();

    Camera3D camera;
    setup_camera(camera);

    GameContext context;
	resetGame(context);
	
    Renderer renderer(camera, context);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- Update systems ---
        ecs_systems::playerMoveControl(context, dt);
        ecs_systems::playerShootControl(context);
        ecs_systems::playerAimTarget(context);
        ecs_systems::enemyMoveControl(context, dt);
        ecs_systems::enemyAimTarget(context);
        ecs_systems::entityMovement(context, dt);
        ecs_systems::entityCollision(context, dt);
        ecs_systems::hpRegen(context, dt);
        ecs_systems::entityLifetime(context, dt);
        ecs_systems::hpCleanup(context);
        ecs_systems::enemyRespawn(context);
        ecs_systems::cleanOutOfBound(context);
        ecs_systems::updatePlayerTargetable(context);
        ecs_systems::asteroidRespawn(context);
		ecs_systems::bulletTargetAim(context);
        ecs_systems::ammoReload(context, dt);
        ecs_systems::bulletWeaponShoot(context, dt);
		ecs_systems::model_rotation_sync(context);

        camaraFollowPlayer(context, camera);
        renderer.Render();

		if (IsKeyPressed(KEY_R)) {
			resetGame(context);
		}		

        DrawFPS(10, 10);
    }
	context.meshManager.unloadAll();
    CloseWindow();
    return 0;
}
