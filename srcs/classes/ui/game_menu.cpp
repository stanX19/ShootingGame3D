#include "game_menu.hpp"
#include "engine.hpp"
#include "entities.hpp"

namespace {
	void spawnInitialMenuScene(GameContext &context)
	{
		spawnSunAndStars(context);
	}
}

GameMenu::GameMenu(GameContext &context)
	: m_context(context),
	  m_renderer(context),
	  m_startButton("START GAME", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  m_hangarButton("HANGAR", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  m_settingsButton("SETTINGS", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20)
{
}

GameMenu::~GameMenu() {}

EngineState GameMenu::run()
{
	if (m_context.registry.storage<entt::entity>().empty())
		spawnInitialMenuScene(m_context);

	// Basic camera setup for menu background
	float arenaSize = m_context.config.ARENA_SIZE;
	m_context.mainCamera.position = Vector3{arenaSize, arenaSize, arenaSize};
	m_context.mainCamera.target = Vector3{0.0f, 0.0f, 0.0f};
	m_context.mainCamera.up = Vector3{0.0f, 1.0f, 0.0f};
	m_context.mainCamera.fovy = 45.0f;
	m_context.mainCamera.projection = CAMERA_PERSPECTIVE;

	EngineState nextState = EngineState::MENU;

	while (!WindowShouldClose() && nextState == EngineState::MENU)
	{
		float dt = GetFrameTime();

		float time = (float)GetTime() * 0.02f;
		m_context.mainCamera.position.x = m_context.config.ARENA_SIZE * cosf(time);
		m_context.mainCamera.position.z = m_context.config.ARENA_SIZE * sinf(time);

		BeginDrawing();
		ClearBackground(BLACK);
		m_renderer.render(dt, m_context.mainCamera);
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

	m_startButton.setBounds(btnStart);
	m_hangarButton.setBounds(btnHangar);
	m_settingsButton.setBounds(btnSettings);

	if (m_startButton.tickAndDraw())
	{
		nextState = EngineState::GAME;
	}

	if (m_hangarButton.tickAndDraw())
	{
		nextState = EngineState::HANGAR;
	}

	if (m_settingsButton.tickAndDraw())
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
