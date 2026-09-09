#include "shoot_3d.hpp"
#include "game_context.hpp"
#include "classes/hud_manager.hpp"
#include "classes/battlefield_hud_renderer.hpp"
#include "components/unit.hpp"
#include <iostream>

int main() {
	// Set headless/unfocused window config flags if supported
	SetConfigFlags(FLAG_WINDOW_HIDDEN);
	InitWindow(1280, 720, "HUD Toast & Damage Numbers Manual Test");
	SetTargetFPS(60);

	Camera3D camera{};
	camera.position = Vector3{0.0f, 2.0f, 8.0f};
	camera.target = Vector3{0.0f, 0.0f, 0.0f};
	camera.up = Vector3{0.0f, 1.0f, 0.0f};
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	GameContext context;
	const entt::entity player = context.registry.create();
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity targetA = context.registry.create();
	context.registry.emplace<Name>(targetA, "Fighter");

	const entt::entity targetB = context.registry.create();
	context.registry.emplace<Name>(targetB, "Elite");

	const entt::entity targetC = context.registry.create();
	context.registry.emplace<Name>(targetC, "Mothership");

	// Add top notification: LOW HP WARNING (Red)
	context.hudManager.addToastTopNotif("LOW HP WARNING", ToastPriority::CRITICAL, RED);

	// Add left log kills
	context.hudManager.addToastLeftLog("Killed Fighter", GREEN);
	context.hudManager.addToastLeftLog("Killed Mothership", GREEN);

	// Report damage:
	// Normal hit: White
	context.hudManager.reportDamage(player, targetA, 85.0f, Vector3{-2.5f, 0.0f, 0.0f}, HitType::NORMAL);
	// Critical hit: Orange
	context.hudManager.reportDamage(player, targetB, 240.0f, Vector3{0.0f, 0.0f, 0.0f}, HitType::CRITICAL);
	// Kill shot: Red
	context.hudManager.reportDamage(player, targetC, 500.0f, Vector3{2.5f, 0.0f, 0.0f}, HitType::KILL);
	BattlefieldHUDRenderer hudRenderer(camera, context);

	// Render a few frames to advance update and drawing
	for (int frame = 0; frame < 5; ++frame) {
		BeginDrawing();
		ClearBackground(Color{15, 15, 25, 255}); // Dark space background

		context.hudManager.update(0.016f, context);
		hudRenderer.renderAll(0.016f);

		EndDrawing();
	}

	// Capture screenshot to assets/snapshots/hud_toast_test.png
	TakeScreenshot("assets/snapshots/hud_toast_test.png");
	std::cout << "HUD manual test screenshot saved to assets/snapshots/hud_toast_test.png\n";

	CloseWindow();
	return 0;
}
