#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "shoot_3d.hpp"
#include "game.hpp"
#include "game_menu.hpp"

enum class EngineState {
    MENU,
    GAME,
    EXIT
};

class Engine {
public:
    Engine();
    ~Engine();

    void run();

private:
    GameContext context;
    EngineState state = EngineState::MENU;

    void init();
    void shutdown();
};

#endif // ENGINE_HPP
