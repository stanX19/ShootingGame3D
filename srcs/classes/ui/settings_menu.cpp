#include "settings_menu.hpp"

SettingsMenu::SettingsMenu(GameContext &context)
    : m_context(context),
      m_hpToggleWidget("HP BAR: ON", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
      m_volumeWidget(
          "VOLUME",
          context.config.settings.masterVolume,
          0.0f,
          1.0f,
          0.1f,
          Rectangle{0.0f, 0.0f, 0.0f, 0.0f},
          SKYBLUE
      ),
      m_sensitivityWidget(
          "SENSITIVITY",
          context.config.settings.controlSensitivity,
          0.01f,
          1.0f,
          0.01f,
          Rectangle{0.0f, 0.0f, 0.0f, 0.0f},
          SKYBLUE
      ),
      m_backWidget("BACK", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, GRAY, 20)
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
    m_context.config.saveChanged();
    
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
    m_hpToggleWidget.setBounds(hpBounds);
    m_hpToggleWidget.setText(m_context.config.settings.showHPBar ? "HP BAR: ON" : "HP BAR: OFF");
    if (m_hpToggleWidget.tickAndDraw()) {
        m_context.config.setBool("settings.showHPBar", !m_context.config.settings.showHPBar);
    }

    const Rectangle volumeBounds = {
        (float)screenWidth / 2 - buttonWidth / 2,
        (float)startY + spacing,
        buttonWidth,
        buttonHeight
    };
    m_volumeWidget.setBounds(volumeBounds);
    if (m_volumeWidget.tickAndDraw()) {
        m_context.soundManager.setMasterVolume(m_context.config.settings.masterVolume);
        m_context.config.setFloat("audio.masterVolume", m_context.config.settings.masterVolume);
    }

    const Rectangle sensitivityBounds = {
        (float)screenWidth / 2 - buttonWidth / 2,
        (float)startY + spacing * 2,
        buttonWidth,
        buttonHeight
    };
    m_sensitivityWidget.setBounds(sensitivityBounds);
    if (m_sensitivityWidget.tickAndDraw()) {
        m_context.config.setFloat("settings.controlSensitivity", m_context.config.settings.controlSensitivity);
    }

    const Rectangle backBounds = {
        (float)screenWidth / 2 - buttonWidth / 2,
        (float)screenHeight * 4 / 5,
        buttonWidth,
        buttonHeight
    };
    m_backWidget.setBounds(backBounds);
    if (m_backWidget.tickAndDraw()) {
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
