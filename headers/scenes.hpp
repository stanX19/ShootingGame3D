#ifndef SCENES_HPP
#define SCENES_HPP

#include "game_state_manager.hpp"

class UIHelper {
public:
    static void DrawButton(const char* text, float x, float y, float width, float height, 
                          bool isSelected, bool isEnabled = true);
    static void DrawCenteredText(const char* text, float y, int fontSize, Color color);
    static void DrawBackground(float animationTime);
    static void HandleMenuNavigation(int& selectedOption, int maxOptions);
};
// Main Menu Scene
class MainMenuScene : public Scene {
public:
    MainMenuScene();
    void OnEnter(GameStateManager* manager) override;
    void Update(GameStateManager* manager, float deltaTime) override;
    void Render(GameStateManager* manager) override;

private:
    int selectedMenuOption;
    int maxMenuOptions;
    float animationTime;
    
    void HandleMenuSelection(GameStateManager* manager);
};

// Game Playing Scene
class GamePlayingScene : public Scene {
public:
    void Update(GameStateManager* manager, float deltaTime) override;
    void Render(GameStateManager* manager) override;

private:
    void UpdateGameSystems(GameStateManager* manager, float deltaTime);
    void UpdateCameraFollow(GameStateManager* manager, float deltaTime);
};

// Pause Menu Scene
class PauseMenuScene : public Scene {
public:
    PauseMenuScene();
    void OnEnter(GameStateManager* manager) override;
    void Update(GameStateManager* manager, float deltaTime) override;
    void Render(GameStateManager* manager) override;

private:
    int selectedMenuOption;
    int maxMenuOptions;
    
    void HandleMenuSelection(GameStateManager* manager);
};

// Game Over Scene
class GameOverScene : public Scene {
public:
    GameOverScene();
    void OnEnter(GameStateManager* manager) override;
    void Update(GameStateManager* manager, float deltaTime) override;
    void Render(GameStateManager* manager) override;

private:
    int selectedMenuOption;
    int maxMenuOptions;
    float animationTime;
    
    void HandleMenuSelection(GameStateManager* manager);
};

// Settings Scene
class SettingsScene : public Scene {
public:
    SettingsScene();
    void OnEnter(GameStateManager* manager) override;
    void Update(GameStateManager* manager, float deltaTime) override;
    void Render(GameStateManager* manager) override;

private:
    int selectedMenuOption;
    int maxMenuOptions;
    
    void HandleMenuSelection(GameStateManager* manager);
    void HandleVolumeAdjustment(GameStateManager* manager, float deltaTime);
};

// Controls Help Scene
class ControlsHelpScene : public Scene {
public:
    void Update(GameStateManager* manager, float deltaTime) override;
    void Render(GameStateManager* manager) override;

private:
    float animationTime = 0.0f;
};

#endif // SCENES_HPP