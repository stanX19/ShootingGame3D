#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "shoot_3d.hpp"
#include "game.hpp"
#include "game_menu.hpp"
#include "game_hangar.hpp"

enum class EngineState {
    MENU,
    GAME,
    HANGAR,
    SETTINGS,
    EXIT
};

class Engine {
public:
    Engine();
    ~Engine();

    void run();

private:
    GameContext m_context;
    EngineState m_state = EngineState::MENU;

    void init();
    void shutdown();
};

#endif // ENGINE_HPP
