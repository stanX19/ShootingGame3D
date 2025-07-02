#include "shoot_3d.hpp"
#include "renderer.hpp"

static void spawnSunAndStars(GameContext &context) {
	entt::entity sun2 = context.registry.create();
	Position pos = {randomUnitVector3() * ARENA_SIZE * 3};
	// t_model_id sunModel = context.meshManager.loadModel("assets/Models/sun/sun.glb");
	t_model_id sunModel = context.meshManager.createSphere(128, 128);
	float rad = GetRandomValue(ARENA_SIZE * 0.8, ARENA_SIZE * 1.5);

	context.registry.emplace<Position>(sun2, pos);
	context.registry.emplace<RenderBody>(sun2, sunModel, rad, Color{105, 205, 255, 255});
	context.registry.emplace<tag::LightSource>(sun2, rad, Color{105, 205, 255, 255});
	
	// stars
	t_model_id starsModel = context.meshManager.createSphere();

	for (int i = 0; i < 100; i++) {
		entt::entity entity = context.registry.create();

		context.registry.emplace<Position>(entity, randomUnitVector3() * ARENA_SIZE * 10);
		context.registry.emplace<RenderBody>(entity, starsModel, (GetRandomValue(30, 50) * ARENA_SIZE / 4000.0f), WHITE);
	}
}

static void setup_camera(Camera3D& camera) {
    camera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

static void camaraFollowPlayer(GameContext &context, Camera3D &camera, float dt) {
    auto playerView = context.registry.view<tag::Player, Position, Rotation>();
    for (auto entity : playerView) {
        Position& pos = playerView.get<Position>(entity);
        Rotation& rot = playerView.get<Rotation>(entity);
        Vector3 forward = GetForwardVector(rot);
        Vector3 up = GetUpVector(rot);

        // Choose follow direction
        bool shift = IsKeyDown(KEY_RIGHT_SHIFT);
        Vector3 desiredPosition = pos.value + (shift ? forward * 10.0f : forward * -10.0f) + up * 5.0f;
        Vector3 desiredTarget   = pos.value + (shift ? forward * -10.0f : forward * 10.0f);

        float smoothing = 12.0f;  // greater --> greater lerp
        float lerp = 1.0f - std::exp(-smoothing * dt);

        // Smooth camera motion
        camera.position = Vector3Lerp(camera.position, desiredPosition, lerp);
        camera.target = Vector3Lerp(camera.target, desiredTarget, lerp);
        camera.up = Vector3Lerp(camera.up, up, lerp);
    }
}



static void resetGame(GameContext &context) {
    context.registry.clear();
    spawnPlayer(context);
	spawnSunAndStars(context);
	SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
}

int main() {
    InitWindow(1600, 900, "3D Space Shooter");
    SetTargetFPS(60);
    // HideCursor();

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
		ecs_systems::entityAnchor(context);
        ecs_systems::entityCollision(context, dt);
		
        ecs_systems::entityLifetime(context, dt);
        ecs_systems::delayedDamage(context, dt);
        ecs_systems::hpCleanup(context);
        ecs_systems::hpRegen(context, dt);
        ecs_systems::cleanOutOfBound(context);
		ecs_systems::entityAnchorRelease(context, dt);
        ecs_systems::enemyRespawn(context);
        ecs_systems::updatePlayerTargetable(context); // TODO: remove this and put into camera
        ecs_systems::asteroidRespawn(context);

        ecs_systems::ammoReload(context, dt);
		ecs_systems::bulletTargetAim(context);
		ecs_systems::weaponParentControlAim(context);
		ecs_systems::weaponParentControlShoot(context);
		ecs_systems::weaponUpdateCooldown(context, dt);
		ecs_systems::weaponUpdateCanFire(context);
        ecs_systems::bulletWeaponShoot(context);
		ecs_systems::weaponUpdateFireStatus(context);

		ecs_systems::syncModelRotation(context);
        camaraFollowPlayer(context, camera, dt);
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
