#include "shoot_3d.hpp"
#include "renderer.hpp"

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
	context.registry.emplace<RenderBody>(entity, context.meshManager.createSphere());
	context.registry.emplace<HP>(entity, HP{hp});
	context.registry.emplace<Damage>(entity, Damage{damagePerSecond});
	context.registry.emplace<tag::Shaded>(entity);
	context.registry.emplace<tag::Targetable>(entity);

	return entity;
}

static void resetGame(GameContext &context) {
	context.registry.clear();
	context.registry.emplace<tag::Player>(context.registry.create());
	spawnBody(context, {3, 0, 0}, {-0.5, 0, 0});
	spawnBody(context, {-3, 0, 0}, {0.5, 0, 0});
	spawnBody(context, {-50, 0, 0}, {500000000, 0, 0});
	event::utils::hookAllListeners(context);
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

		ecs_systems::entityMovement(context, dt);
		ecs_systems::detectEntityCollision(context, dt);
		context.dispatcher.update();

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
