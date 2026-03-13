#ifndef SETTINGS_MENU_HPP
#define SETTINGS_MENU_HPP

#include "shoot_3d.hpp"
#include "engine.hpp"

class SettingsMenu {
public:
    SettingsMenu(GameContext &context);
    ~SettingsMenu();

    EngineState run();

private:
    GameContext &context;

    void drawSettingsUI(EngineState &nextState);
    void inputControls(float dt, EngineState &nextState);
};

#endif // SETTINGS_MENU_HPP
