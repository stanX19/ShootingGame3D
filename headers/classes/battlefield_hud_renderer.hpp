#ifndef BATTLEFIELD_HUD_RENDERER_HPP
#define BATTLEFIELD_HUD_RENDERER_HPP

#include "includes.hpp"
#include "components.hpp"
#include "components/factions.hpp"
#include "utils.hpp"
#include "game_context.hpp"

class BattlefieldHUDRenderer {
public:
    BattlefieldHUDRenderer(Camera3D &camera, GameContext &context);

    void Render(float dt);

private:
    Camera3D &camera;
    GameContext &context;
    float currentDt;

    void drawHUD();
    void drawHealthBars();
    void drawTargetable();
    void drawTexts();
    void drawSpeedBar();
    void drawThrustBar();
    void drawAmmoCircle();
    void drawAimCircle();
    void drawCrosshair();
    void drawMainUIFrame();
    void drawCursorArrow();
    void drawCollisionWarning();

    Vector2 GetUIFrameCenter() const;
    float GetUIFrameRadius() const;
};

#endif
