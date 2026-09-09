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
    GameContext &m_context;
    Renderer m_renderer;
    entt::entity m_previewPlayer;

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
    ) const;

    std::vector<std::string> m_standardWeapons;
    std::vector<std::string> m_specialWeapons;
    std::vector<ui::TextButtonWidget> m_turretButtons;
    ui::ScrollableListWidget m_turretList;
    std::vector<std::string> m_shipIds;
    std::size_t m_selectedShipIndex = 0;
    std::string m_selectedShipId;

    ui::TextButtonWidget m_specialButton;
    ui::TextButtonWidget m_shipButton;
    ui::TextButtonWidget m_backButton;

    void cycleWeapon(const std::string& path, std::string &currentWeapon, const std::vector<std::string> &options);
};

#endif // GAME_HANGAR_HPP
