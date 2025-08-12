#include "scenes.hpp"
#include <algorithm>
#include <cmath>

// ============================================================================
// UI Helper Implementation
// ============================================================================

void UIHelper::DrawButton(const char* text, float x, float y, float width, float height, 
                         bool isSelected, bool isEnabled) {
    Color bgColor = isSelected ? ColorAlpha(SKYBLUE, 0.3f) : ColorAlpha(DARKGRAY, 0.3f);
    Color borderColor = isSelected ? SKYBLUE : GRAY;
    Color textColor = isEnabled ? (isSelected ? WHITE : LIGHTGRAY) : GRAY;
    
    if (!isEnabled) {
        bgColor = ColorAlpha(DARKGRAY, 0.1f);
        borderColor = DARKGRAY;
    }
    
    // Button background
    DrawRectangle(x, y, width, height, bgColor);
    DrawRectangleLines(x, y, width, height, borderColor);
    
    // Button text
    int textWidth = MeasureText(text, 24);
    DrawText(text, x + width/2 - textWidth/2, y + height/2 - 12, 24, textColor);
    
    // Selection indicator
    if (isSelected && isEnabled) {
        DrawRectangle(x - 5, y + height/2 - 10, 3, 20, SKYBLUE);
        DrawRectangle(x + width + 2, y + height/2 - 10, 3, 20, SKYBLUE);
    }
}

void UIHelper::DrawCenteredText(const char* text, float y, int fontSize, Color color) {
    int textWidth = MeasureText(text, fontSize);
    DrawText(text, GetScreenWidth() / 2 - textWidth / 2, y, fontSize, color);
}

void UIHelper::DrawBackground(float animationTime) {
    // Animated starfield background
    for (int i = 0; i < 100; i++) {
        float x = (i * 37.0f + animationTime * 10.0f);
        x = fmodf(x, GetScreenWidth() + 100) - 50;
        
        float y = (i * 47.0f + animationTime * 15.0f);
        y = fmodf(y, GetScreenHeight() + 100) - 50;
        
        float alpha = 0.3f + 0.4f * sinf(i + animationTime);
        DrawCircle(x, y, 1 + (i % 3), ColorAlpha(WHITE, alpha));
    }
    
    // Grid effect
    for (int x = 0; x < GetScreenWidth(); x += 50) {
        DrawLine(x, 0, x, GetScreenHeight(), ColorAlpha(DARKBLUE, 0.1f));
    }
    for (int y = 0; y < GetScreenHeight(); y += 50) {
        DrawLine(0, y, GetScreenWidth(), y, ColorAlpha(DARKBLUE, 0.1f));
    }
}

void UIHelper::HandleMenuNavigation(int& selectedOption, int maxOptions) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        selectedOption = (selectedOption - 1 + maxOptions) % maxOptions;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        selectedOption = (selectedOption + 1) % maxOptions;
    }
}

// ============================================================================
// Main Menu Scene Implementation
// ============================================================================

MainMenuScene::MainMenuScene() 
    : selectedMenuOption(0)
    , maxMenuOptions(4)
    , animationTime(0.0f)
{
}

void MainMenuScene::OnEnter([[maybe_unused]] GameStateManager* manager) {
    selectedMenuOption = 0;
    animationTime = 0.0f;
}

void MainMenuScene::Update([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    animationTime += deltaTime;
    
    UIHelper::HandleMenuNavigation(selectedMenuOption, maxMenuOptions);
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        HandleMenuSelection(manager);
    }
    
    if (IsKeyPressed(KEY_BACKSPACE)) {
        manager->RequestClose();
    }
}

void MainMenuScene::Render([[maybe_unused]] GameStateManager* manager) {
    BeginDrawing();
    ClearBackground(BLACK);
    
    UIHelper::DrawBackground(animationTime * 0.5f);
    
    // Title
    const char* title = "3D SPACE SHOOTER";
    int titleWidth = MeasureText(title, 80);
    float titleY = GetScreenHeight() * 0.2f;
    DrawText(title, GetScreenWidth() / 2 - titleWidth / 2, titleY, 80, RAYWHITE);
    
    // Subtitle with animation
    float pulseAlpha = 0.7f + 0.3f * sinf(animationTime * 2.0f);
    const char* subtitle = "Survive the void, conquer the stars";
    int subtitleWidth = MeasureText(subtitle, 24);
    DrawText(subtitle, GetScreenWidth() / 2 - subtitleWidth / 2, titleY + 100, 24, 
             ColorAlpha(SKYBLUE, pulseAlpha));
    
    // Menu buttons
    float startY = GetScreenHeight() * 0.5f;
    float buttonHeight = 60.0f;
    float buttonWidth = 300.0f;
    float buttonX = GetScreenWidth() / 2 - buttonWidth / 2;
    
    UIHelper::DrawButton("START GAME", buttonX, startY, buttonWidth, buttonHeight, selectedMenuOption == 0);
    UIHelper::DrawButton("CONTROLS", buttonX, startY + 80, buttonWidth, buttonHeight, selectedMenuOption == 1);
    UIHelper::DrawButton("SETTINGS", buttonX, startY + 160, buttonWidth, buttonHeight, selectedMenuOption == 2);
    UIHelper::DrawButton("EXIT", buttonX, startY + 240, buttonWidth, buttonHeight, selectedMenuOption == 3);
    
    // Instructions
    UIHelper::DrawCenteredText("Use ARROW KEYS or WASD to navigate, ENTER to select", 
                              GetScreenHeight() - 60, 20, LIGHTGRAY);
    
    if (!manager->IsFirstGame()) {
        char scoreText[64];
        sprintf(scoreText, "Last Score: %d", manager->GetFinalScore());
        UIHelper::DrawCenteredText(scoreText, GetScreenHeight() - 100, 24, ORANGE);
    }
    
    EndDrawing();
}

void MainMenuScene::HandleMenuSelection([[maybe_unused]] GameStateManager* manager) {
    switch (selectedMenuOption) {
        case 0: // Start Game
            manager->ResetGame();
            manager->SetGameTime(0.0f);
            manager->SetFirstGame(false);
            manager->ChangeState(GameState::PLAYING);
            break;
        case 1: // Controls
            manager->ChangeState(GameState::CONTROLS_HELP);
            break;
        case 2: // Settings
            manager->ChangeState(GameState::SETTINGS);
            break;
        case 3: // Exit
            manager->RequestClose();
            break;
    }
}

// ============================================================================
// Game Playing Scene Implementation
// ============================================================================

void GamePlayingScene::Update([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    manager->AddGameTime(deltaTime);
    
    // Handle pause
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_P)) {
        manager->ChangeState(GameState::PAUSED);
        return;
    }
    
    // Handle reset
    if (IsKeyPressed(KEY_R)) {
        manager->ResetGame();
        return;
    }
    
    // Handle self-destruct (from original code)
    if (IsKeyPressed(KEY_DELETE) && manager->GetGameContext().registry.valid(manager->GetGameContext().currentPlayer)) {
        manager->GetGameContext().registry.emplace<DelayedDamage>(manager->GetGameContext().currentPlayer, DelayedDamage{0.0f, 100000000.0f});
    }
    
    // Center cursor
    if (IsKeyPressed(KEY_C)) {
        SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    }
    
    UpdateGameSystems(manager, deltaTime);
    UpdateCameraFollow(manager, deltaTime);
    
    // Check if player is dead
    if (!manager->GetGameContext().registry.valid(manager->GetGameContext().currentPlayer)) {
        // Save final score
        Score* scorePtr = manager->GetGameContext().registry.try_get<Score>(manager->GetGameContext().currentPlayer);
        if (scorePtr) {
            manager->SetFinalScore(scorePtr->value);
        }
        manager->ChangeState(GameState::GAME_OVER);
    }
}

void GamePlayingScene::Render([[maybe_unused]] GameStateManager* manager) {
    manager->GetRenderer()->Render();
}

void GamePlayingScene::UpdateGameSystems([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    GameContext& context = manager->GetGameContext();
    Camera3D& camera = manager->GetCamera();
    
    // Update game systems (from original main.cpp)
    ecs_systems::playerMoveControl(context, deltaTime, camera);
    ecs_systems::playerShootControl(context);
    ecs_systems::playerAimTarget(context);
    ecs_systems::enemyMoveControl(context, deltaTime);
    ecs_systems::enemyAimTarget(context);

    ecs_systems::ammoReload(context, deltaTime);
    ecs_systems::bulletTargetAim(context);
    ecs_systems::weaponParentControlAim(context);
    ecs_systems::weaponParentControlShoot(context);
    ecs_systems::weaponUpdateCooldown(context, deltaTime);
    ecs_systems::weaponUpdateCanFire(context);
    ecs_systems::weaponShoot(context);
    ecs_systems::weaponUpdateFireStatus(context);

    ecs_systems::entityMovement(context, deltaTime);
    ecs_systems::entityAnchor(context);
    ecs_systems::entityTransformation(context, deltaTime);
    ecs_systems::detectEntityCollision(context, deltaTime);
    context.dispatcher.update();

    ecs_systems::syncModelRotation(context);

    // Cleanup systems
    ecs_systems::entityLifetime(context, deltaTime);
    ecs_systems::delayedDamage(context, deltaTime);
    ecs_systems::hpCleanup(context);
    ecs_systems::hpRegen(context, deltaTime);
    ecs_systems::cleanOutOfBound(context);
    ecs_systems::entityAnchorRelease(context, deltaTime);
    ecs_systems::enemyRespawn(context);
    ecs_systems::asteroidRespawn(context);
}

void GamePlayingScene::UpdateCameraFollow([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    GameContext& context = manager->GetGameContext();
    Camera3D& camera = manager->GetCamera();
    
    if (!context.registry.valid(context.currentPlayer))
        return;
    
    Position& pos = context.registry.get<Position>(context.currentPlayer);
    Rotation& rot = context.registry.get<Rotation>(context.currentPlayer);
    Vector3 forward = getForwardVector(rot);
    Vector3 up = getUpVector(rot);
    
    bool shift = IsKeyDown(KEY_RIGHT_SHIFT);
    float k = 10.0f;
    Vector3 desiredPosition = pos.value + (shift ? forward * k : forward * -k) + up * 5.0f;
    Vector3 desiredTarget = pos.value + (shift ? forward * -(20 - k) : forward * (20 - k) + up * 1.0f);
    
    float smoothing = 12.0f;
    float lerp = 1.0f - std::exp(-smoothing * deltaTime);
    camera.position = Vector3Lerp(camera.position, desiredPosition, lerp);
    camera.target = Vector3Lerp(camera.target, desiredTarget, lerp);
    camera.up = Vector3Lerp(camera.up, up, lerp * 0.2f);
}

// ============================================================================
// Pause Menu Scene Implementation
// ============================================================================

PauseMenuScene::PauseMenuScene() 
    : selectedMenuOption(0)
    , maxMenuOptions(3)
{
}

void PauseMenuScene::OnEnter([[maybe_unused]] GameStateManager* manager) {
    selectedMenuOption = 0;
}

void PauseMenuScene::Update([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    UIHelper::HandleMenuNavigation(selectedMenuOption, maxMenuOptions);
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        HandleMenuSelection(manager);
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_P)) {
        manager->ChangeState(GameState::PLAYING);
    }
}

void PauseMenuScene::Render([[maybe_unused]] GameStateManager* manager) {
    manager->GetRenderer()->Render();
    
    BeginDrawing();
    // Dark overlay
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.7f));
    
    // Pause menu
    UIHelper::DrawCenteredText("GAME PAUSED", GetScreenHeight() * 0.3f, 60, RAYWHITE);
    
    float startY = GetScreenHeight() * 0.5f;
    float buttonHeight = 50.0f;
    float buttonWidth = 250.0f;
    float buttonX = GetScreenWidth() / 2 - buttonWidth / 2;
    
    UIHelper::DrawButton("RESUME", buttonX, startY, buttonWidth, buttonHeight, selectedMenuOption == 0);
    UIHelper::DrawButton("SETTINGS", buttonX, startY + 70, buttonWidth, buttonHeight, selectedMenuOption == 1);
    UIHelper::DrawButton("MAIN MENU", buttonX, startY + 140, buttonWidth, buttonHeight, selectedMenuOption == 2);
    
    UIHelper::DrawCenteredText("ESC or P to resume", GetScreenHeight() - 60, 20, LIGHTGRAY);
    
    EndDrawing();
}

void PauseMenuScene::HandleMenuSelection([[maybe_unused]] GameStateManager* manager) {
    switch (selectedMenuOption) {
        case 0: // Resume
            manager->ChangeState(GameState::PLAYING);
            break;
        case 1: // Settings
            manager->ChangeState(GameState::SETTINGS);
            break;
        case 2: // Main Menu
            manager->ChangeState(GameState::MAIN_MENU);
            break;
    }
}

// ============================================================================
// Game Over Scene Implementation
// ============================================================================

GameOverScene::GameOverScene() 
    : selectedMenuOption(0)
    , maxMenuOptions(2)
    , animationTime(0.0f)
{
}

void GameOverScene::OnEnter([[maybe_unused]] GameStateManager* manager) {
    selectedMenuOption = 0;
    animationTime = 0.0f;
}

void GameOverScene::Update([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    animationTime += deltaTime;
    
    UIHelper::HandleMenuNavigation(selectedMenuOption, maxMenuOptions);
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        HandleMenuSelection(manager);
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_R)) {
        manager->ResetGame();
        manager->SetGameTime(0.0f);
        manager->ChangeState(GameState::PLAYING);
    }
}

void GameOverScene::Render([[maybe_unused]] GameStateManager* manager) {
    BeginDrawing();
    ClearBackground(BLACK);
    
    UIHelper::DrawBackground(animationTime * 0.5f);
    
    // Game Over title
    UIHelper::DrawCenteredText("GAME OVER", GetScreenHeight() * 0.25f, 80, RED);
    
    // Final score
    char scoreText[64];
    sprintf(scoreText, "Final Score: %d", manager->GetFinalScore());
    UIHelper::DrawCenteredText(scoreText, GetScreenHeight() * 0.4f, 50, ORANGE);
    
    // Time survived
    char timeText[64];
    int minutes = (int)(manager->GetGameTime() / 60);
    int seconds = (int)manager->GetGameTime() % 60;
    sprintf(timeText, "Time Survived: %02d:%02d", minutes, seconds);
    UIHelper::DrawCenteredText(timeText, GetScreenHeight() * 0.5f, 30, SKYBLUE);
    
    // Menu buttons
    float startY = GetScreenHeight() * 0.6f;
    float buttonHeight = 60.0f;
    float buttonWidth = 300.0f;
    float buttonX = GetScreenWidth() / 2 - buttonWidth / 2;
    
    UIHelper::DrawButton("PLAY AGAIN", buttonX, startY, buttonWidth, buttonHeight, selectedMenuOption == 0);
    UIHelper::DrawButton("MAIN MENU", buttonX, startY + 80, buttonWidth, buttonHeight, selectedMenuOption == 1);
    
    UIHelper::DrawCenteredText("R to restart, ESC for main menu", GetScreenHeight() - 60, 20, LIGHTGRAY);
    
    EndDrawing();
}

void GameOverScene::HandleMenuSelection([[maybe_unused]] GameStateManager* manager) {
    switch (selectedMenuOption) {
        case 0: // Play Again
            manager->ResetGame();
            manager->SetGameTime(0.0f);
            manager->ChangeState(GameState::PLAYING);
            break;
        case 1: // Main Menu
            manager->ChangeState(GameState::MAIN_MENU);
            break;
    }
}

// ============================================================================
// Settings Scene Implementation
// ============================================================================

SettingsScene::SettingsScene() 
    : selectedMenuOption(0)
    , maxMenuOptions(4)
{
}

void SettingsScene::OnEnter([[maybe_unused]] GameStateManager* manager) {
    selectedMenuOption = 0;
}

void SettingsScene::Update([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    UIHelper::HandleMenuNavigation(selectedMenuOption, maxMenuOptions);
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        HandleMenuSelection(manager);
    }
    
    HandleVolumeAdjustment(manager, deltaTime);
    
    if (IsKeyPressed(KEY_BACKSPACE)) {
        manager->ChangeState(manager->GetPreviousState());
    }
}

void SettingsScene::Render([[maybe_unused]] GameStateManager* manager) {
    BeginDrawing();
    ClearBackground(BLACK);
    
    UIHelper::DrawBackground(0.0f);
    
    UIHelper::DrawCenteredText("SETTINGS", GetScreenHeight() * 0.2f, 60, RAYWHITE);
    
    float startY = GetScreenHeight() * 0.4f;
    float itemHeight = 80.0f;
    float labelX = GetScreenWidth() * 0.3f;
    float valueX = GetScreenWidth() * 0.7f;
    
    // Sound setting
    Color soundColor = selectedMenuOption == 0 ? YELLOW : WHITE;
    DrawText("Sound:", labelX, startY, 30, soundColor);
    DrawText(manager->IsSoundEnabled() ? "ON" : "OFF", valueX, startY, 30, 
             manager->IsSoundEnabled() ? GREEN : RED);
    
    // Volume setting
    Color volumeColor = selectedMenuOption == 1 ? YELLOW : WHITE;
    DrawText("Volume:", labelX, startY + itemHeight, 30, volumeColor);
    char volumeText[16];
    sprintf(volumeText, "%.0f%%", manager->GetMasterVolume() * 100);
    DrawText(volumeText, valueX, startY + itemHeight, 30, volumeColor);
    
    // Volume bar
    if (selectedMenuOption == 1) {
        float barY = startY + itemHeight + 35;
        float barWidth = 200.0f;
        float barX = valueX - 50;
        DrawRectangle(barX, barY, barWidth, 8, DARKGRAY);
        DrawRectangle(barX, barY, barWidth * manager->GetMasterVolume(), 8, SKYBLUE);
        DrawText("< >", barX + barWidth + 10, barY - 5, 20, LIGHTGRAY);
    }
    
    // Graphics setting
    Color graphicsColor = selectedMenuOption == 2 ? YELLOW : WHITE;
    DrawText("Graphics:", labelX, startY + itemHeight * 2, 30, graphicsColor);
    const char* qualityNames[] = {"Low", "Medium", "High"};
    DrawText(qualityNames[manager->GetGraphicsQuality()], valueX, startY + itemHeight * 2, 30, graphicsColor);
    
    // Back button
    float buttonWidth = 200.0f;
    float buttonX = GetScreenWidth() / 2 - buttonWidth / 2;
    UIHelper::DrawButton("BACK", buttonX, startY + itemHeight * 3, buttonWidth, 50, selectedMenuOption == 3);
    
    UIHelper::DrawCenteredText("ESC to go back", GetScreenHeight() - 60, 20, LIGHTGRAY);
    
    EndDrawing();
}

void SettingsScene::HandleMenuSelection([[maybe_unused]] GameStateManager* manager) {
    switch (selectedMenuOption) {
        case 0: // Sound Toggle
            manager->SetSoundEnabled(!manager->IsSoundEnabled());
            break;
        case 1: // Volume (handled in HandleVolumeAdjustment)
            break;
        case 2: // Graphics Quality
            manager->SetGraphicsQuality((manager->GetGraphicsQuality() + 1) % 3);
            break;
        case 3: // Back
            manager->ChangeState(manager->GetPreviousState());
            break;
    }
}

void SettingsScene::HandleVolumeAdjustment([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    if (selectedMenuOption == 1) {
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            float newVolume = std::max(0.0f, manager->GetMasterVolume() - deltaTime);
            manager->SetMasterVolume(newVolume);
        }
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
            float newVolume = std::min(1.0f, manager->GetMasterVolume() + deltaTime);
            manager->SetMasterVolume(newVolume);
        }
    }
}

// ============================================================================
// Controls Help Scene Implementation
// ============================================================================

void ControlsHelpScene::Update([[maybe_unused]] GameStateManager* manager, [[maybe_unused]] float deltaTime) {
    animationTime += deltaTime;
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_BACKSPACE)) {
        manager->ChangeState(GameState::MAIN_MENU);
    }
}

void ControlsHelpScene::Render([[maybe_unused]] GameStateManager* manager) {
    BeginDrawing();
    ClearBackground(BLACK);
    
    UIHelper::DrawBackground(animationTime * 0.5f);
    
    UIHelper::DrawCenteredText("CONTROLS", GetScreenHeight() * 0.1f, 60, RAYWHITE);
    
    float startY = GetScreenHeight() * 0.25f;
    float lineHeight = 40.0f;
    float leftCol = GetScreenWidth() * 0.2f;
    float rightCol = GetScreenWidth() * 0.6f;
    
    // Movement controls
    DrawText("MOVEMENT:", leftCol, startY, 24, SKYBLUE);
    DrawText("W/S or Right Click", rightCol, startY, 20, WHITE);
    
    DrawText("STEERING:", leftCol, startY + lineHeight, 24, SKYBLUE);
    DrawText("Arrow Keys or Mouse", rightCol, startY + lineHeight, 20, WHITE);
    
    DrawText("SHOOTING:", leftCol, startY + lineHeight * 2, 24, SKYBLUE);
    DrawText("Space or Left Click", rightCol, startY + lineHeight * 2, 20, WHITE);
    
    // Game controls
    DrawText("GAME CONTROLS:", leftCol, startY + lineHeight * 4, 24, ORANGE);
    DrawText("P or ESC - Pause", rightCol, startY + lineHeight * 4, 20, WHITE);
    DrawText("R - Restart Game", rightCol, startY + lineHeight * 5, 20, WHITE);
    DrawText("C - Center Cursor", rightCol, startY + lineHeight * 6, 20, WHITE);
    DrawText("Delete - Self Destruct", rightCol, startY + lineHeight * 7, 20, WHITE);
    
    // Camera controls
    DrawText("CAMERA:", leftCol, startY + lineHeight * 9, 24, GREEN);
    DrawText("Right Shift - Reverse View", rightCol, startY + lineHeight * 9, 20, WHITE);
    
    // Back button
    float buttonWidth = 200.0f;
    float buttonX = GetScreenWidth() / 2 - buttonWidth / 2;
    UIHelper::DrawButton("BACK", buttonX, GetScreenHeight() - 120, buttonWidth, 50, true);
    
    UIHelper::DrawCenteredText("Press any key to return", GetScreenHeight() - 40, 20, LIGHTGRAY);
    
    EndDrawing();
}