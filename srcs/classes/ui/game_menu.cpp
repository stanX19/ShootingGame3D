#include "game_menu.hpp"
#include "engine.hpp"

GameMenu::GameMenu(GameContext &context)
	: context(context),
	  renderer(context.mainCamera, context),
	  startButton("START GAME", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  hangarButton("HANGAR", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  settingsButton("SETTINGS", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20)
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
	Rectangle btnHangar = {(float)screenWidth / 2 - 100, (float)screenHeight / 2 + 35, 200, 50};
	Rectangle btnSettings = {(float)screenWidth / 2 - 100, (float)screenHeight / 2 + 95, 200, 50};

	startButton.setBounds(btnStart);
	hangarButton.setBounds(btnHangar);
	settingsButton.setBounds(btnSettings);

	if (startButton.tick_and_draw())
	{
		nextState = EngineState::GAME;
	}

	if (hangarButton.tick_and_draw())
	{
		nextState = EngineState::HANGAR;
	}

	if (settingsButton.tick_and_draw())
	{
		nextState = EngineState::SETTINGS;
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
