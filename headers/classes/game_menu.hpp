#ifndef GAME_MENU_HPP
#define GAME_MENU_HPP

#include "shoot_3d.hpp"
#include "renderer.hpp"

enum class EngineState; // Forward declaration

class GameMenu {
public:
    GameMenu(GameContext &context);
    ~GameMenu();

    EngineState run();

private:
    GameContext &context;
    Renderer renderer;

    void drawMenuUI(EngineState &nextState);
	void inputControls([[maybe_unused]] float dt, EngineState &nextState);
};

#endif // GAME_MENU_HPP
