#include "game_state_manager.hpp"

int main() {
    InitWindow(1600, 900, "3D Space Shooter");
    SetTargetFPS(60);
    // HideCursor(); // Uncomment if you want to hide cursor during gameplay

    GameStateManager gameManager;

    while (!WindowShouldClose() && !gameManager.ShouldClose()) {
        float deltaTime = GetFrameTime();
        
        gameManager.Update(deltaTime);
        gameManager.Render();
    }

    // Cleanup is handled automatically by GameStateManager destructor
    CloseWindow();
    return 0;
}