#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP
#include "includes.hpp"
#include "game_state_manager.hpp"

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

#endif // MAIN_MENU_HPP