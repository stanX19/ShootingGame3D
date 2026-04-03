#include "settings_menu.hpp"

SettingsMenu::SettingsMenu(GameContext &context)
    : context(context),
      hpToggleWidget("HP BAR: ON", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, LIME, 20),
      volumeWidget(
          "VOLUME",
          context.config.settings.masterVolume,
          0.0f,
          1.0f,
          0.1f,
          Rectangle{0.0f, 0.0f, 0.0f, 0.0f},
          SKYBLUE
      ),
      sensitivityWidget(
          "SENSITIVITY",
          context.config.settings.controlSensitivity,
          0.01f,
          2.0f,
          0.01f,
          Rectangle{0.0f, 0.0f, 0.0f, 0.0f},
          SKYBLUE
      ),
      backWidget("BACK", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, GRAY, 20)
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

    const Rectangle hpBounds = {(float)screenWidth / 2 - buttonWidth / 2, (float)startY, buttonWidth, buttonHeight};
    hpToggleWidget.setBounds(hpBounds);
    hpToggleWidget.setText(context.config.settings.showHPBar ? "HP BAR: ON" : "HP BAR: OFF");
    hpToggleWidget.setColor(context.config.settings.showHPBar ? LIME : RED);
    if (hpToggleWidget.tick_and_draw()) {
        context.config.setBool("settings.showHPBar", !context.config.settings.showHPBar);
    }

    const Rectangle volumeBounds = {
        (float)screenWidth / 2 - buttonWidth / 2,
        (float)startY + spacing,
        buttonWidth,
        buttonHeight
    };
    volumeWidget.setBounds(volumeBounds);
    if (volumeWidget.tick_and_draw()) {
        context.soundManager.setMasterVolume(context.config.settings.masterVolume);
        context.config.setFloat("audio.masterVolume", context.config.settings.masterVolume);
    }

    const Rectangle sensitivityBounds = {
        (float)screenWidth / 2 - buttonWidth / 2,
        (float)startY + spacing * 2,
        buttonWidth,
        buttonHeight
    };
    sensitivityWidget.setBounds(sensitivityBounds);
    if (sensitivityWidget.tick_and_draw()) {
        context.config.setFloat("settings.controlSensitivity", context.config.settings.controlSensitivity);
    }

    const Rectangle backBounds = {
        (float)screenWidth / 2 - buttonWidth / 2,
        (float)screenHeight * 4 / 5,
        buttonWidth,
        buttonHeight
    };
    backWidget.setBounds(backBounds);
    if (backWidget.tick_and_draw()) {
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
