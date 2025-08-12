#include "game_state_manager.hpp"
#include "scenes.hpp"

GameStateManager::GameStateManager() 
    : currentState(GameState::MAIN_MENU)
    , previousState(GameState::MAIN_MENU)
    , finalScore(0)
    , gameTime(0.0f)
    , isFirstGame(true)
    , shouldClose(false)
    , soundEnabled(true)
    , masterVolume(0.8f)
    , graphicsQuality(1)
{
    InitializeGame();
    InitializeScenes();
    
    // Enter initial state
    if (scenes[currentState]) {
        scenes[currentState]->OnEnter(this);
    }
}

GameStateManager::~GameStateManager() {
    scenes.clear();
    renderer.reset();
}

void GameStateManager::InitializeGame() {
    SetupCamera();
    renderer = std::make_unique<Renderer>(camera, gameContext);
}

void GameStateManager::SetupCamera() {
    camera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void GameStateManager::InitializeScenes() {
    scenes[GameState::MAIN_MENU] = std::make_unique<MainMenuScene>();
    scenes[GameState::PLAYING] = std::make_unique<GamePlayingScene>();
    scenes[GameState::PAUSED] = std::make_unique<PauseMenuScene>();
    scenes[GameState::GAME_OVER] = std::make_unique<GameOverScene>();
    scenes[GameState::SETTINGS] = std::make_unique<SettingsScene>();
    scenes[GameState::CONTROLS_HELP] = std::make_unique<ControlsHelpScene>();
}

void GameStateManager::Update(float deltaTime) {
    if (scenes[currentState]) {
        scenes[currentState]->Update(this, deltaTime);
    }
}

void GameStateManager::Render() {
    if (scenes[currentState]) {
        scenes[currentState]->Render(this);
    }
}

void GameStateManager::ChangeState(GameState newState) {
    // Exit current state
    if (scenes[currentState]) {
        scenes[currentState]->OnExit(this);
    }
    
    previousState = currentState;
    currentState = newState;
    
    // Enter new state
    if (scenes[currentState]) {
        scenes[currentState]->OnEnter(this);
    }
}

void GameStateManager::ResetGame() {
    gameContext.registry.clear();
    event::utils::hookAllListeners(gameContext);
    SetupCamera();
    spawnPlayer(gameContext);
    spawnSunAndStars(gameContext);
    SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    gameTime = 0.0f;
}