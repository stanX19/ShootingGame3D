#ifndef GAME_HPP
#define GAME_HPP

#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "battlefield_hud_renderer.hpp"

enum class EngineState; // Forward declaration

class Game {
public:
    Game(GameContext &context);
    ~Game();

    void reset();
    EngineState run();

private:
    GameContext &context;
    Renderer renderer;
    BattlefieldHUDRenderer hudRenderer;

    void inputControls([[maybe_unused]] float dt, EngineState &nextState);
};

#endif // GAME_HPP
