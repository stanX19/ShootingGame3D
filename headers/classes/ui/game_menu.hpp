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
    GameContext &m_context;
    Renderer m_renderer;
    ui::TextButtonWidget m_startButton;
    ui::TextButtonWidget m_hangarButton;
    ui::TextButtonWidget m_settingsButton;

    void drawMenuUI(EngineState &nextState);
	void inputControls([[maybe_unused]] float dt, EngineState &nextState);
};

#endif // GAME_MENU_HPP
