#ifndef BATTLEFIELD_HUD_RENDERER_HPP
#define BATTLEFIELD_HUD_RENDERER_HPP

#include "includes.hpp"
#include "components/physics.hpp"
#include "components/spaceship.hpp"
#include "components/combat.hpp"
#include "components/collision.hpp"
#include "components/weapon.hpp"
#include "components/identity.hpp"
#include "components/faction.hpp"
#include "utils.hpp"
#include "game_context.hpp"

class BattlefieldHUDRenderer {
public:
    BattlefieldHUDRenderer(Camera3D &camera, GameContext &context);

    void renderAll(float dt);
    void RenderAll(float dt) { renderAll(dt); }

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
    void drawMissileWarning();
    void drawDamageNumbers(const Camera3D &camera);
    void drawToasts();
    void drawWarningToastItem(const HudManager::ActiveToast &t, Vector2 slotPos, float alpha);

    Vector2 getUIFrameCenter() const;
    float getUIFrameRadius() const;
    Vector2 GetUIFrameCenter() const { return getUIFrameCenter(); }
    float GetUIFrameRadius() const { return getUIFrameRadius(); }

private:
    Camera3D &m_camera;
    GameContext &m_context;
    float m_currentDt = 0.0f;

    // Encapsulated HUD state
    int m_score = 0;
    float m_animationAngle = 0.0f;
    entt::entity m_prevTargetedEntity = entt::null;
    float m_blinkTimer = 0.0f;
    float m_reloadAngleOffset = 0.0f;
    float m_speedAnimationTime = 0.0f;
};

#endif
