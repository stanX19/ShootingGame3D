#ifndef GAME_HANGAR_HPP
#define GAME_HANGAR_HPP

#include "shoot_3d.hpp"
#include "renderer.hpp"
#include <vector>
#include <string>

enum class EngineState; // Forward declaration

class GameHangar {
public:
    GameHangar(GameContext &context);
    ~GameHangar();

    EngineState run();

private:
    GameContext &context;
    Renderer renderer;
    entt::entity previewPlayer;

    void drawUI(EngineState &nextState);
    void inputControls(float dt, EngineState &nextState);
    void spawnPreviewShip();
    void destroyPreviewShip();

    std::vector<std::string> standardWeapons;
    std::vector<std::string> specialWeapons;
    void cycleWeapon(const std::string& path, std::string &currentWeapon, const std::vector<std::string> &options);
};

#endif // GAME_HANGAR_HPP
