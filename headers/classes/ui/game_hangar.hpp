#ifndef GAME_HANGAR_HPP
#define GAME_HANGAR_HPP

#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "classes/ui/scrollable_list_widget.hpp"
#include "classes/ui/text_button_widget.hpp"
#include <cstddef>
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
    void cycleTurretWeapon(std::size_t index);
    void cycleShip();
    void resetShipLoadout();
    std::size_t selectedMountCount() const;
    void prepareTurretButton(std::size_t index, Rectangle bounds);
    void drawShipPanel();
    void drawShipStats(
        const config::UnitConfig::Definition &definition,
        std::size_t mountCount,
        float panelX,
        float statsY
    );

    std::vector<std::string> standardWeapons;
    std::vector<std::string> specialWeapons;
    std::vector<ui::TextButtonWidget> turretButtons;
    ui::ScrollableListWidget turretList;
    std::vector<std::string> shipIds;
    std::size_t selectedShipIndex = 0;
    std::string selectedShipId;

    ui::TextButtonWidget specialButton;
    ui::TextButtonWidget shipButton;
    ui::TextButtonWidget backButton;

    void cycleWeapon(const std::string& path, std::string &currentWeapon, const std::vector<std::string> &options);
};

#endif // GAME_HANGAR_HPP
