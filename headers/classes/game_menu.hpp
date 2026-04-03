#ifndef GAME_MENU_HPP
#define GAME_MENU_HPP

#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "classes/ui/text_button_widget.hpp"

enum class EngineState; // Forward declaration

class GameMenu {
public:
    GameMenu(GameContext &context);
    ~GameMenu();

    EngineState run();

private:
    GameContext &context;
    Renderer renderer;
    ui::TextButtonWidget startButton;
    ui::TextButtonWidget hangarButton;
    ui::TextButtonWidget settingsButton;

    void drawMenuUI(EngineState &nextState);
	void inputControls([[maybe_unused]] float dt, EngineState &nextState);
};

#endif // GAME_MENU_HPP
