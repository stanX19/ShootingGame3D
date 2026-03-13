#include "settings_menu.hpp"
#include "utils/draw_utils.hpp"

SettingsMenu::SettingsMenu(GameContext &context)
    : context(context)
{
}

SettingsMenu::~SettingsMenu() {}

EngineState SettingsMenu::run()
{
    EngineState nextState = EngineState::SETTINGS;

    while (!WindowShouldClose() && nextState == EngineState::SETTINGS)
    {
        float dt = GetFrameTime();

        BeginDrawing();
        ClearBackground(BLACK);
        
        drawSettingsUI(nextState);
        
        EndDrawing();

        inputControls(dt, nextState);
    }
    
    // Save on exit from settings
    context.config.save("assets/config/game_config.json");
    
    return nextState;
}

void SettingsMenu::drawSettingsUI(EngineState &nextState)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    const char *title = "SETTINGS";
    int titleWidth = MeasureText(title, 40);
    DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 6, 40, SKYBLUE);

    int startY = screenHeight / 3;
    int spacing = 60;

    float buttonWidth = 400;
    float buttonHeight = 40;

    // HP Bar Toggle
    bool showHP = context.config.settings.showHPBar;
    Rectangle btnHP = {(float)screenWidth / 2 - buttonWidth / 2, (float)startY, buttonWidth, buttonHeight};
    const char* hpText = showHP ? "HP BAR: ON" : "HP BAR: OFF";
    if (draw_utils::draw_text_button(hpText, btnHP, showHP ? LIME : RED, 20)) {
        context.config.setBool("settings.showHPBar", !showHP);
    }

    // Volume Control using draw_value_button
    float volume = context.config.settings.masterVolume;
    Rectangle btnVol = {(float)screenWidth / 2 - buttonWidth / 2, (float)startY + spacing, buttonWidth, buttonHeight};
    
    if (draw_utils::draw_value_button("VOLUME", volume, 0.1f, 0.0f, 1.0f, btnVol, SKYBLUE)) {
        context.soundManager.setMasterVolume(volume);
        context.config.setFloat("audio.masterVolume", volume);
    }

    // Back Button
    Rectangle btnBack = {(float)screenWidth / 2 - buttonWidth / 2, (float)screenHeight * 4 / 5, buttonWidth, buttonHeight};
    if (draw_utils::draw_text_button("BACK", btnBack, GRAY, 20)) {
        nextState = EngineState::MENU;
    }
}

void SettingsMenu::inputControls([[maybe_unused]] float dt, EngineState &nextState)
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        nextState = EngineState::MENU;
    }
}
