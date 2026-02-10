#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "battlefield_hud_renderer.hpp"
#include "weapons.hpp"

const float dt = 1.0f;

static void setup_camera(Camera3D& camera) {
	camera.position = Vector3{ 0.0f, 1.0f, 4.0f } * 3;
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
	context.registry.emplace<RenderBody>(entity, context.modelManager.loadModel("assets/Models/bullet/lazer.glb"), radius);
	context.registry.emplace<HP>(entity, HP{hp});
	context.registry.emplace<Damage>(entity, Damage{damagePerSecond});
	context.registry.emplace<ModelStrech>(entity, ModelStrech{1.0f / (2 * radius)});
	context.registry.emplace<tag::Shaded>(entity);
	context.registry.emplace<tag::Targetable>(entity);
	context.registry.emplace<tag::VelocitySyncModelRot>(entity);
	context.registry.emplace<tag::effect::DropDebris>(entity);
	
	return entity;
}

void printEntityStats(GameContext &context) {
	auto view = context.registry.view<PrevPosition, Position, Velocity, HP, ModelStrech>();
	for (auto [entity, prevPos, pos, vel, hp, strech] : view.each()) {
		printf("Entity %d: PrevPos(%.2f, %.2f, %.2f) Pos(%.2f, %.2f, %.2f) Vel(%.2f, %.2f, %.2f) HP(%.2f/%.2f) Strech(%.4f)\n",
		       static_cast<int>(entity),
		       prevPos.value.x, prevPos.value.y, prevPos.value.z,
		       pos.value.x, pos.value.y, pos.value.z,
		       vel.value.x, vel.value.y, vel.value.z,
		       hp.value, hp.maxValue,
		       strech.scale);
	}
}

static void resetGame(GameContext &context) {
	context.registry.clear();
	spawnBody(context, {3, 0, 0}, {0, 0, 0});
	entt::entity gun =  spawnBody(context, {-3, 0, 0}, {0, 0, 0});
	weapon::utils::setUpRegistry(context);
	weapon::emplaceWeaponLazerBasic(context, gun);
	// make sure the weapon is allowed to fire immediately in this test
	if (auto *wc = context.registry.try_get<WeaponCooldown>(gun))
		wc->timeSinceLastShot = 100000.0f;
	// aim toward the other spawned body so bullets are visible
	context.registry.emplace_or_replace<AimDirection>(gun, AimDirection{Vector3{1.0f, 0.0f, 0.0f}});
	context.registry.emplace<tag::weapon::FireRequest>(gun);
	// spawnBody(context, {-3, 0, 0}, {50000, 0, 0}, 0.1f, 0.01f);
	event::utils::hookAllListeners(context);
	ecs_systems::weaponUpdateCanFire(context, dt);
	ecs_systems::weaponShoot(context, dt);
	ecs_systems::syncModelRotation(context, dt);
	ecs_systems::entityMovement(context, dt);
	ecs_systems::detectEntityCollision(context, dt);
	context.dispatcher.update();
	
	printEntityStats(context);
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
	BattlefieldHUDRenderer hudRenderer(camera, context);

	while (!WindowShouldClose()) {
		BeginDrawing();
		renderer.Render(dt);
		hudRenderer.setDt(dt);
		hudRenderer.drawHealthBars();
		EndDrawing();

		if (IsKeyPressed(KEY_R)) {
			resetGame(context);
			printEntityStats(context);
		}
	}
	context.modelManager.unloadAll();
	CloseWindow();
	return 0;
}
