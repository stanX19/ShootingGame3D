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

    void RenderAll(float dt);

	void setDt(float dt);
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

private:
    Camera3D &camera;
    GameContext &context;
    float currentDt;

    Vector2 GetUIFrameCenter() const;
    float GetUIFrameRadius() const;
};

#endif
