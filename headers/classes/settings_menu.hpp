#ifndef SETTINGS_MENU_HPP
#define SETTINGS_MENU_HPP

#include "shoot_3d.hpp"
#include "engine.hpp"
#include "classes/ui/text_button_widget.hpp"
#include "classes/ui/float_stepper_widget.hpp"
#include "classes/ui/float_slider_widget.hpp"

class SettingsMenu {
public:
    SettingsMenu(GameContext &context);
    ~SettingsMenu();

    EngineState run();

private:
    GameContext &context;
    ui::TextButtonWidget hpToggleWidget;
    ui::FloatSliderWidget volumeWidget;
    ui::FloatSliderWidget sensitivityWidget;
    ui::TextButtonWidget backWidget;

    void drawSettingsUI(EngineState &nextState);
    void inputControls(float dt, EngineState &nextState);
};

#endif // SETTINGS_MENU_HPP
