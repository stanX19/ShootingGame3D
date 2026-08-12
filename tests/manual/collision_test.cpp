#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "battlefield_hud_renderer.hpp"

static void setup_camera(Camera3D& camera) {
	camera.position = Vector3{ 0.0f, 1.0f, 4.0f } * 5;
	camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
	camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;
}


entt::entity spawnBody(GameContext &context, const Vector3 &position, const Vector3 &velocity, float radius = 1.0f, float hp = 100.0f, float damagePerSecond = 2.0f)
{
	auto entity = context.registry.create();

	context.registry.emplace<Position>(entity, Position{position});
	context.registry.emplace<Velocity>(entity, Velocity{velocity});
	context.registry.emplace<CollisionBody>(entity, CollisionBody{radius});
	context.registry.emplace<RenderBody>(entity, context.modelManager.createSphere(), radius);
	context.registry.emplace<HP>(entity, HP{hp});
	context.registry.emplace<Damage>(entity, Damage{damagePerSecond});
	context.registry.emplace<Mass>(entity, Mass{radius * radius * radius});
	context.registry.emplace<tag::Shaded>(entity);
	context.registry.emplace<tag::Targetable>(entity);
	context.registry.emplace<tag::effect::DropDebris>(entity);
	
	return entity;
}

static void resetGame(GameContext &context) {
	context.registry.clear();
	spawnBody(context, {3, 0, 0}, {-0.5, 0, 0});
	spawnBody(context, {-3, 0, 0}, {0.5, 0, 0});
	// spawnBody(context, {-50, 0, 0}, {500000000, 0, 0}, 0.001f, 1.0f);
	event::utils::hookAllListeners(context);
}

int main() {
	InitWindow(1600, 900, "3D Space Shooter");
	SetTargetFPS(60);
	DisableCursor();

	Camera3D camera;
	setup_camera(camera);

	GameContext context;
	context.config.init({
		{"audio", "assets/config/audio.json"},
		{"debug", "assets/config/debug.json"},
		{"game", "assets/config/game.json"},
		{"loadout", "assets/config/loadout.json"},
		{"physics", "assets/config/physics.json"},
		{"settings", "assets/config/settings.json"},
		{"sounds", "assets/config/sounds.json"},
		{"units", "assets/config/units.json"},
		{"weapons", "assets/config/weapons.json"},
		{"spaceship", "assets/config/spaceships.json"}
	});
	resetGame(context);
	
	Renderer renderer(camera, context);
	BattlefieldHUDRenderer hudRenderer(camera, context);

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		ecs_systems::entityMovement(context, dt);
		ecs_systems::detectEntityCollision(context, dt);
		context.dispatcher.update();

		BeginDrawing();
		renderer.Render(dt);
		hudRenderer.setDt(dt);
		hudRenderer.drawHealthBars();
		EndDrawing();

		if (IsKeyPressed(KEY_R)) {
			resetGame(context);
		}

		DrawFPS(10, 10);
	}
	context.modelManager.unloadAll();
	CloseWindow();
	return 0;
}
