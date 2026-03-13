#include "game_menu.hpp"
#include "engine.hpp"

GameMenu::GameMenu(GameContext &context)
	: context(context),
	  renderer(context.mainCamera, context)
{
}

GameMenu::~GameMenu() {}

EngineState GameMenu::run()
{
	// Basic camera setup for menu background
	float arenaSize = context.config.ARENA_SIZE;
	context.mainCamera.position = Vector3{arenaSize, arenaSize, arenaSize};
	context.mainCamera.target = Vector3{0.0f, 0.0f, 0.0f};
	context.mainCamera.up = Vector3{0.0f, 1.0f, 0.0f};
	context.mainCamera.fovy = 45.0f;
	context.mainCamera.projection = CAMERA_PERSPECTIVE;

	EngineState nextState = EngineState::MENU;

	while (!WindowShouldClose() && nextState == EngineState::MENU)
	{
		float dt = GetFrameTime();

		float time = (float)GetTime() * 0.02f;
		context.mainCamera.position.x = context.config.ARENA_SIZE * cosf(time);
		context.mainCamera.position.z = context.config.ARENA_SIZE * sinf(time);

		BeginDrawing();
		ClearBackground(BLACK);
		renderer.Render(dt);
		drawMenuUI(nextState);
		EndDrawing();

		inputControls(dt, nextState);
	}
	return nextState;
}

void GameMenu::drawMenuUI(EngineState &nextState)
{
	int screenWidth = GetScreenWidth();
	int screenHeight = GetScreenHeight();

	const char *title = "3D SPACE SHOOTER";
	int titleWidth = MeasureText(title, 60);
	DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 4, 60, SKYBLUE);

	Rectangle btnStart = {(float)screenWidth / 2 - 100, (float)screenHeight / 2 - 25, 200, 50};
	bool hovered = CheckCollisionPointRec(GetMousePosition(), btnStart);

	DrawRectangleRec(btnStart, hovered ? SKYBLUE : DARKBLUE);
	DrawRectangleLinesEx(btnStart, 2, WHITE);

	const char *btnText = "START GAME";
	int textWidth = MeasureText(btnText, 20);
	DrawText(btnText, screenWidth / 2 - textWidth / 2, screenHeight / 2 - 10, 20, WHITE);

	if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		nextState = EngineState::GAME;
	}

	const char *hint = "Press ESC to Exit";
	int hintWidth = MeasureText(hint, 20);
	DrawText(hint, screenWidth / 2 - hintWidth / 2, screenHeight * 3 / 4, 20, GRAY);
}

void GameMenu::inputControls([[maybe_unused]] float dt, EngineState &nextState)
{
	if (IsKeyPressed(KEY_ESCAPE))
	{
		nextState = EngineState::EXIT;
	}
	if (IsKeyPressed(KEY_ENTER))
	{
		nextState = EngineState::GAME;
	}
}
