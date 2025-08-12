#ifndef GAME_STATE_MANAGER_HPP
#define GAME_STATE_MANAGER_HPP

#include "shoot_3d.hpp"
#include "renderer.hpp"
#include <memory>
#include <unordered_map>

enum class GameState {
    MAIN_MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    SETTINGS,
    CONTROLS_HELP
};

// Forward declarations
class Scene;
class GameStateManager;

// Scene interface
class Scene {
public:
    virtual ~Scene() = default;
    virtual void OnEnter([[maybe_unused]] GameStateManager* manager) {}
    virtual void OnExit([[maybe_unused]] GameStateManager* manager) {}
    virtual void Update([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) = 0;
    virtual void Render([[maybe_unused]] GameStateManager* manager) = 0;
};

// Game State Manager - now much simpler
class GameStateManager {
public:
    GameStateManager();
    ~GameStateManager();
    
    // Main game loop functions
    void Update(float deltaTime);
    void Render();
    
    // State management
    void ChangeState(GameState newState);
    GameState GetCurrentState() const { return currentState; }
    GameState GetPreviousState() const { return previousState; }
    
    // Game component access
    GameContext& GetGameContext() { return gameContext; }
    Camera3D& GetCamera() { return camera; }
    std::unique_ptr<Renderer>& GetRenderer() { return renderer; }
    
    // Game data access
    int GetFinalScore() const { return finalScore; }
    void SetFinalScore(int score) { finalScore = score; }
    float GetGameTime() const { return gameTime; }
    void SetGameTime(float time) { gameTime = time; }
    void AddGameTime(float deltaTime) { gameTime += deltaTime; }
    bool IsFirstGame() const { return isFirstGame; }
    void SetFirstGame(bool first) { isFirstGame = first; }
    
    // Settings access
    bool IsSoundEnabled() const { return soundEnabled; }
    void SetSoundEnabled(bool enabled) { soundEnabled = enabled; }
    float GetMasterVolume() const { return masterVolume; }
    void SetMasterVolume(float volume) { masterVolume = volume; }
    int GetGraphicsQuality() const { return graphicsQuality; }
    void SetGraphicsQuality(int quality) { graphicsQuality = quality; }
    
    // Game control
    void ResetGame();
    void SetupCamera();
    
    // Exit control
    bool ShouldClose() const { return shouldClose; }
    void RequestClose() { shouldClose = true; }

private:
    // Current state
    GameState currentState;
    GameState previousState;
    
    // Scenes
    std::unordered_map<GameState, std::unique_ptr<Scene>> scenes;
    
    // Game components
    GameContext gameContext;
    Camera3D camera;
    std::unique_ptr<Renderer> renderer;
    
    // Game data
    int finalScore;
    float gameTime;
    bool isFirstGame;
    bool shouldClose;
    
    // Settings
    bool soundEnabled;
    float masterVolume;
    int graphicsQuality;
    
    // Initialize scenes
    void InitializeScenes();
    void InitializeGame();
};

#endif // GAME_STATE_MANAGER_HPP