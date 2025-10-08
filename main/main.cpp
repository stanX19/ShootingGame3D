#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "battlefield_hud_renderer.hpp"
#include "entt_utils.hpp"

static void setup_camera(Camera3D& camera) {
	camera.position = Vector3{ 0.0f, 1.0f, 4.0f };
	camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
	camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;
}

static void camaraFollowPlayer(GameContext &context, Camera3D &camera, [[maybe_unused]] float  dt) {
    if (!context.registry.valid(context.currentPlayer))
        return;
    
    Position& pos = context.registry.get<Position>(context.currentPlayer);
    Rotation& rot = context.registry.get<Rotation>(context.currentPlayer);
    Vector3 forward = getForwardVector(rot);
    Vector3 up = getUpVector(rot);
	
    bool shift = IsKeyDown(KEY_RIGHT_SHIFT);
    float k = 10.0f;
    Vector3 desiredPosition = pos.value + (shift ? forward * k : forward * -k) + up * 4.0f;
    Vector3 desiredTarget = pos.value + (shift ? forward * -(20 - k) : forward * (20 - k) + up * 2.0f);
    
    // Velocity& vel = context.registry.get<Velocity>(context.currentPlayer);
    float smoothing = 12.0f;
	float lerp = 1.0f - std::exp(-smoothing * dt);
    camera.position = Vector3Lerp(camera.position, desiredPosition, lerp);
    camera.target = Vector3Lerp(camera.target, desiredTarget, lerp);
    camera.up = Vector3Lerp(camera.up, up, lerp * 0.2f);
}

static void resetGame(GameContext &context, Camera &camera) {
	context.registry.clear();
	weapon::utils::setUpRegistry(context);
	event::utils::hookAllListeners(context);
	setup_camera(camera);
	spawnPlayer(context);
	spawnSunAndStars(context);
	SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
}

static void inputControls(GameContext &context, Camera &camera, [[maybe_unused]] float dt) {
	if (IsKeyPressed(KEY_R)) {
		resetGame(context, camera);
	}
	if (IsKeyPressed(KEY_DELETE) && context.registry.valid(context.currentPlayer)) {
		context.registry.emplace<DelayedDamage>(context.currentPlayer, DelayedDamage{0.0f, 100000000.0f});
		spawnPlayer(context);
	}
	if (IsKeyPressed(KEY_C)) {
		SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
	}
}

int main() {
	InitWindow(1600, 900, "3D Space Shooter");
	SetTargetFPS(60);
	// HideCursor();

	Camera3D camera;
	GameContext context;
	
	resetGame(context, camera);
	
	Renderer renderer(camera, context);
	BattlefieldHUDRenderer hudRenderer(camera, context);

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		// --- Update systems ---
		ecs_systems::playerMoveControl(context, dt, camera);
		ecs_systems::playerShootControl(context);
		// ecs_systems::playerRespawn(context);
		ecs_systems::aiFindTarget(context);
		ecs_systems::aiMoveControl(context, dt);
		ecs_systems::aiShootControl(context);

		ecs_systems::ammoReload(context, dt);
		ecs_systems::bulletTargetAim(context);
		ecs_systems::weaponParentControlAim(context);
		ecs_systems::weaponParentControlShoot(context);
		ecs_systems::weaponUpdateCooldown(context, dt);
		ecs_systems::weaponUpdateCanFire(context);
		ecs_systems::weaponShoot(context);
		ecs_systems::weaponUpdateFireStatus(context);

		ecs_systems::entityMovement(context, dt);
		ecs_systems::entityAnchor(context, dt);
		ecs_systems::entityTransformation(context, dt);
		ecs_systems::detectEntityCollision(context, dt);
		context.dispatcher.update();

		ecs_systems::energyShield(context, dt);
		
		ecs_systems::syncModelRotation(context);
		camaraFollowPlayer(context, camera, dt);
		BeginDrawing();
		renderer.Render();
		hudRenderer.Render();
		EndDrawing();

		ecs_systems::blueUnitRespawn(context);
		ecs_systems::redUnitRespawn(context);
		ecs_systems::asteroidRespawn(context);
		ecs_systems::entityAnchorRelease(context, dt);
		ecs_systems::entityLifetime(context, dt);
		ecs_systems::cleanOutOfBound(context);
		ecs_systems::delayedDamage(context, dt);
		ecs_systems::hpCleanup(context);
		ecs_systems::hpRegen(context, dt);

		inputControls(context, camera, dt);
	}
	context.modelManager.unloadAll();
	CloseWindow();
	return 0;
}
