#include "battlefield_hud_renderer.hpp"
#include "draw_utils.hpp"
#include <algorithm>
#include <numeric>
#include <tuple>
#include <vector>
#include <cmath>
#include <cstdio>

using namespace physics;
using namespace combat;
using namespace collision;
using namespace spaceship;
using namespace identity;
using namespace weapon;

BattlefieldHUDRenderer::BattlefieldHUDRenderer(Camera3D &camera, GameContext &context)
    : m_camera(camera), m_context(context), m_currentDt(0.0f)
{
}

void BattlefieldHUDRenderer::renderAll(float dt)
{
    setDt(dt);
    drawHUD();
    drawTexts();
    drawDamageNumbers(m_camera);
    drawToasts();
}

void BattlefieldHUDRenderer::setDt(float dt)
{
    m_currentDt = dt;
}

void BattlefieldHUDRenderer::drawHUD()
{
    drawHealthBars();
    drawTargetable();

    if (!m_context.registry.valid(m_context.currentPlayer))
        return;

    drawAimCircle();
    drawMainUIFrame();
    drawSpeedBar();
    // drawThrustBar();
    drawAmmoCircle();
    drawCrosshair();
    drawCursorArrow();
    drawCollisionWarning();
    drawMissileWarning();
}

void BattlefieldHUDRenderer::drawTexts()
{
    DrawFPS(10, 10);

    if (m_context.registry.valid(m_context.currentPlayer))
    {
        Color textColor = SKYBLUE;
        int totalEntities = 0;
        auto hittableView = m_context.registry.view<CollisionBody, Position, HP>();
        for (auto entity : hittableView)
        {
            if (hittableView.get<HP>(entity).value > 0)
            {
                totalEntities++;
            }
        }
        DrawText(TextFormat("Entities: %d", totalEntities), 10, 30, 20, textColor);
		if (IsKeyDown(KEY_H)) {
			DrawText("Move: W S or Right click", 10, 50, 20, textColor);
			DrawText("Turn: Arrows or Mouse cursor", 10, 70, 20, textColor);
			DrawText("Fire: Space or Left click", 10, 90, 20, textColor);
		}

		auto playerFacPtr = m_context.registry.try_get<faction::Faction>(m_context.currentPlayer);

		if (playerFacPtr) {
			int allyKill = std::accumulate(m_context.factions.begin(), m_context.factions.end(), 0, [&](int acc, const std::pair<const faction::FacVal, FactionData> &pair) {
				if (pair.first != playerFacPtr->value)
					return acc + pair.second.deaths;
				return acc;
			});
			int enemyKill = m_context.factions[playerFacPtr->value].deaths;
			char buf[40];
			float screenCenterX = GetScreenWidth() / 2.0f;
			int spacing = 20;
			int textSize = 24;
			// int textSpacing = 50;
			int textWidth = MeasureText("Kills", textSize);
			DrawText("Kills", screenCenterX - textWidth / 2, 10, textSize, GRAY);
			// DrawText("Allied", screenCenterX - MeasureText("Allied", textSize) - spacing - textSpacing, 10, textSize, SKYBLUE);
			sprintf(buf, "%i", allyKill);
			DrawText(buf, screenCenterX - MeasureText(buf, textSize) - spacing - textWidth / 2, 10, textSize, SKYBLUE);
			// DrawText("Enemy", screenCenterX + spacing + textSpacing, 10, textSize, ORANGE);
			sprintf(buf, "%i", enemyKill);
			DrawText(buf, screenCenterX + spacing + textWidth / 2, 10, textSize, ORANGE);
		}
    }
    else
    {
        const char *msg = "GAME OVER - PRESS R TO RESTART";
        int w = MeasureText(msg, 40);
        DrawText(msg, GetScreenWidth() / 2 - w / 2, GetScreenHeight() / 2 + 50, 40, RED);

        char buf[40];
        sprintf(buf, "Final Score: %i", m_score);
        w = MeasureText(buf, 50);
        DrawText(buf, GetScreenWidth() / 2 - w / 2, GetScreenHeight() / 2, 50, ORANGE);
    }
}

void BattlefieldHUDRenderer::drawHealthBars()
{
	if (!m_context.config.settings.showHPBar)
		return;

    auto view = m_context.registry.view<Position, CollisionBody, HP, combat::tag::Targetable>();
    for (auto entity : view)
    {
        auto &pos = view.get<Position>(entity);
        auto &hp = view.get<HP>(entity);
        EnergyShield *shieldPtr = m_context.registry.try_get<EnergyShield>(entity);

        if (hp.value == hp.maxValue && (!shieldPtr || shieldPtr->hp == shieldPtr->maxHp))
            continue;
        if (!draw_utils::isInFrontOfCamera(pos.value, m_camera))
            continue;
        Vector2 screen = GetWorldToScreen(pos.value, m_camera);

        screen.y -= 20;

        if (screen.x < 0 || screen.x > GetScreenWidth() ||
            screen.y < 0 || screen.y > GetScreenHeight())
            continue;

        float w = 40, h = 4;
        float pct = (float)hp.value / hp.maxValue;

        DrawRectangle(screen.x - w / 2 - 1, screen.y - 1, w + 2, h + 2, DARKGRAY);
        DrawRectangle(screen.x - w / 2, screen.y, w, h, GRAY);
        DrawRectangle(screen.x - w / 2, screen.y, w * pct, h, GREEN);

        if (!shieldPtr || shieldPtr->activeTimer <= 0.0f)
            continue;
        pct = (float)shieldPtr->hp / shieldPtr->maxHp;

        DrawRectangle(screen.x - w / 2 - 1, screen.y - 1 + h + 1, w + 2, h + 2, DARKGRAY);
        DrawRectangle(screen.x - w / 2, screen.y + h + 1, w, h, GRAY);
        DrawRectangle(screen.x - w / 2, screen.y + h + 1, w * pct, h, SKYBLUE);
    }
}

void BattlefieldHUDRenderer::drawTargetable()
{
    auto [aimTargetPtr, playerPosPtr, playerFacPtr] = m_context.registry.try_get<AimTarget, Position, faction::Faction>(m_context.currentPlayer);
    entt::entity targetedEntity = aimTargetPtr ? aimTargetPtr->entity : entt::null;
    Vector3 playerPos = playerPosPtr ? playerPosPtr->value : m_camera.target;
    faction::FacVal playerFac = playerFacPtr ? playerFacPtr->value : 0;

    Vector3 camForward = Vector3Normalize(m_camera.target - m_camera.position);
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, m_camera.up));
    Vector3 camUp = Vector3CrossProduct(camRight, camForward);

    Vector2 screenCenter = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    float uiFrameRadius = getUIFrameRadius();

    m_animationAngle = wrapAngleDegree(m_animationAngle + 1.0f);

    const float blinkInterval = 0.1f;

    m_blinkTimer -= m_currentDt;
    if (targetedEntity != m_prevTargetedEntity) {
        m_blinkTimer = blinkInterval * 3;
    }
    m_prevTargetedEntity = targetedEntity;
    std::vector<std::tuple<entt::entity, Vector3, float, faction::FacVal>> allPosArr;
    std::vector<std::tuple<entt::entity, Vector3, float, faction::FacVal>> posArr;
    for (auto [entity, pos, faction] : m_context.registry.view<Position, combat::tag::Targetable, faction::Faction>().each())
    {
        if (entity == m_context.currentPlayer)
            continue;
        float dist = Vector3Distance(pos.value, playerPos);
        faction::FacVal isAlly = faction.value & playerFac;
        if (dist < m_context.config.COMBAT_DIST * 2.5f)
            posArr.push_back({entity, pos.value, dist, isAlly});
        else
            allPosArr.push_back({entity, pos.value, dist, isAlly});
    }
    std::sort(allPosArr.begin(), allPosArr.end(), [&](const auto &p1, const auto &p2) {
        return std::get<2>(p1) < std::get<2>(p2);
    });

    if (posArr.size() < 3) {
        int toAdd = std::min(allPosArr.size(), 3 - posArr.size());
        posArr.insert(posArr.end(), allPosArr.begin(), allPosArr.begin() + toAdd);
    }

    for (auto [entity, pos, distance, isAlly]: posArr) {
        Vector3 toTarget = pos - playerPos;
        Vector3 local = Vector3{
            Vector3DotProduct(toTarget, camRight),
            Vector3DotProduct(toTarget, camUp),
            Vector3DotProduct(toTarget, camForward)
        };

        bool behind = local.z <= 0;

        Color color = isAlly ? SKYBLUE : RED;

        Vector2 screenPos = GetWorldToScreen(pos, m_camera);

        if (behind)
        {
            screenPos.x = local.x;
            screenPos.y = local.y;
            screenPos.x *= 1e9;
            screenPos.y *= 1e9;
        }

        if (behind || screenPos.x < 0 || screenPos.x > GetScreenWidth() || screenPos.y < 0 || screenPos.y > GetScreenHeight())
        {
            if (isAlly)
                continue;

            Vector2 relToCenter = screenPos - screenCenter;
            Vector2 unitDir = Vector2Normalize(relToCenter);
            Vector2 arrowLoc = screenCenter + unitDir * (uiFrameRadius + 20);
            Vector2 left = { -unitDir.y, unitDir.x };

            DrawTriangle(
                arrowLoc + unitDir * 10,
                arrowLoc - left * 5,
                arrowLoc + left * 5,
                color);
            continue;
        }

        DrawCircleLines(screenPos.x, screenPos.y, 15, color);
        DrawCircleLines(screenPos.x, screenPos.y, 16, color);

        if (entity == targetedEntity)
        {
            float innerRad = 17 + 500.0f / distance;
            Color aimColor = MAROON;

            if (m_blinkTimer >= 0 && fmod(m_blinkTimer / blinkInterval, 2.0f) >= 1.0f) {
                aimColor = ColorAlpha(aimColor, 0.3f);
            }

            DrawRingLines(screenPos, innerRad, innerRad + 2, 90 + m_animationAngle, 180 + m_animationAngle, 12, aimColor);
            DrawRingLines(screenPos, innerRad, innerRad + 2, 270 + m_animationAngle, 360 + m_animationAngle, 12, aimColor);
            DrawLine(screenPos.x + innerRad + 2, screenPos.y, screenPos.x + innerRad + 7, screenPos.y, aimColor);
            DrawLine(screenPos.x - innerRad - 2, screenPos.y, screenPos.x - innerRad - 7, screenPos.y, aimColor);
            DrawLine(screenPos.x, screenPos.y + innerRad + 2, screenPos.x, screenPos.y + innerRad + 7, aimColor);
            DrawLine(screenPos.x, screenPos.y - innerRad - 2, screenPos.x, screenPos.y - innerRad - 7, aimColor);
        }

        if (!isAlly) {
            char txt[32];
            if (distance < 1000)
                snprintf(txt, sizeof(txt), "%.1fm", distance);
            else
                snprintf(txt, sizeof(txt), "%.2fkm", distance / 1000.0f);
            DrawText(txt, screenPos.x + 20, screenPos.y + 10, 20, MAROON);
        }
    }
}

void BattlefieldHUDRenderer::drawSpeedBar()
{
    if (!m_context.registry.all_of<Velocity, MaxSpeed, Rotation>(m_context.currentPlayer))
        return;

    entt::entity entity = m_context.currentPlayer;
    const auto& velocity = m_context.registry.get<Velocity>(entity);
    const auto& maxSpeed = m_context.registry.get<MaxSpeed>(entity);
    const auto& rotation = m_context.registry.get<Rotation>(entity);

    float currentSpeed = Vector3DotProduct(velocity.value, getForwardVector(rotation));
    float speedRatio = currentSpeed / (maxSpeed.value * 2);
    speedRatio = std::min(1.0, speedRatio > 0.5 ? 0.8 + 0.2 * ((speedRatio - 0.5) / 0.5) : speedRatio / 0.5 * 0.8);

    Vector2 center = getUIFrameCenter();
    float frameRadius = getUIFrameRadius();

    float speedBarRadius = frameRadius + 13;
    float speedBarThickness = 8;
    float startAngle = 240.0f - 90.0f;
    float maxAngleRange = 50.0f;
    float currentAngleRange = maxAngleRange * speedRatio;

    DrawRingLines(center, speedBarRadius - speedBarThickness/2, speedBarRadius + speedBarThickness/2,
                  startAngle, startAngle + maxAngleRange, 32, ColorAlpha(DARKGRAY, 0.8f));
    DrawRingLines(center, speedBarRadius - speedBarThickness/2 + 1, speedBarRadius + speedBarThickness/2 - 1,
                  startAngle, startAngle + maxAngleRange, 32, ColorAlpha(BLACK, 0.6f));

    Color speedColor = speedRatio > 0.8 ? ColorLerp(ORANGE, RED, (speedRatio - 0.8) / 0.2) : SKYBLUE;
    if (speedRatio > 0) {
        DrawRingLines(center, speedBarRadius - speedBarThickness/2 + 1, speedBarRadius + speedBarThickness/2 - 1,
                      startAngle, startAngle + currentAngleRange, 32, speedColor);
    }
    if (speedRatio > 0.7f) {
        DrawRingLines(center, speedBarRadius - speedBarThickness/2, speedBarRadius + speedBarThickness/2,
                      startAngle, startAngle + currentAngleRange, 32, ColorAlpha(speedColor, 0.5f));
    }

    float labelAngle = startAngle + maxAngleRange;
    float labelAngleRad = labelAngle * DEG2RAD;
    float labelRadius = speedBarRadius + speedBarThickness;
    Vector2 labelPos = {center.x + cosf(labelAngleRad) * labelRadius - MeasureText("SPEED", 11),
                       center.y + sinf(labelAngleRad) * labelRadius - 8};
    DrawText("SPEED", labelPos.x, labelPos.y, 11, WHITE);

    char speedText[16];
    snprintf(speedText, sizeof(speedText), "%.0f", currentSpeed);
    Vector2 valuePos = {center.x + cosf(labelAngleRad) * labelRadius - MeasureText(speedText, 14),
                        center.y + sinf(labelAngleRad) * labelRadius + 10};
    DrawText(speedText, valuePos.x, valuePos.y, 14, speedColor);

    if (maxSpeed.value > 0) {
        float safeSpeedRatio = 0.8f;
        float safeAngle = startAngle + (maxAngleRange * safeSpeedRatio);
        float safeAngleRad = safeAngle * DEG2RAD;

        Vector2 safeInner = {center.x + cosf(safeAngleRad) * (speedBarRadius - speedBarThickness/2 - 2),
                            center.y + sinf(safeAngleRad) * (speedBarRadius - speedBarThickness/2 - 2)};
        Vector2 safeOuter = {center.x + cosf(safeAngleRad) * (speedBarRadius + speedBarThickness/2 + 2),
                            center.y + sinf(safeAngleRad) * (speedBarRadius + speedBarThickness/2 + 2)};

        DrawLineEx(safeInner, safeOuter, 3.0f, BLUE);
    }
}

void BattlefieldHUDRenderer::drawThrustBar()
{
    if (!m_context.registry.all_of<Velocity>(m_context.currentPlayer))
        return;

    float thrustRatio = 0.7f;

    Vector2 barPos = {GetScreenWidth() - 70.0f, GetScreenHeight() - 150.0f};
    Vector2 barSize = {20.0f, 100.0f};

    DrawRectangleRounded({barPos.x - 2, barPos.y - 2, barSize.x + 4, barSize.y + 4}, 0.2f, 8, ColorAlpha(DARKGRAY, 0.8f));
    DrawRectangleRounded({barPos.x, barPos.y, barSize.x, barSize.y}, 0.2f, 8, ColorAlpha(BLACK, 0.6f));

    float fillHeight = barSize.y * thrustRatio;
    Color thrustColor = thrustRatio > 0.8f ? YELLOW : (thrustRatio > 0.5f ? ORANGE : BLUE);
    DrawRectangleRounded({barPos.x, barPos.y + barSize.y - fillHeight, barSize.x, fillHeight}, 0.2f, 8, thrustColor);

    DrawText("THR", barPos.x - 5, barPos.y - 25, 16, WHITE);

    char thrustText[16];
    snprintf(thrustText, sizeof(thrustText), "%.0f%%", thrustRatio * 100);
    DrawText(thrustText, barPos.x - 15, barPos.y + barSize.y + 5, 14, WHITE);
}

void BattlefieldHUDRenderer::drawAimCircle()
{
    if (!m_context.registry.valid(m_context.currentPlayer))
        return;

    std::vector<Vector2> aimLocs;

    auto addWeaponAim = [&](entt::entity entity, Vector3 pos, Vector3 aim) {
        float dist = m_context.config.COMBAT_DIST;
        auto targetPtr = m_context.registry.try_get<AimTarget>(entity);
        if (targetPtr) {
            auto targetPosPtr = m_context.registry.try_get<Position>(targetPtr->entity);
            if (targetPosPtr)
                dist = Vector3Distance(pos, targetPosPtr->value);
        }
        Vector3 position = pos + aim * dist;
        aimLocs.push_back(GetWorldToScreen(position, m_camera));
    };

    if (m_context.registry.all_of<Position, AimDirection>(m_context.currentPlayer)) {
        auto [pos, aim] = m_context.registry.get<Position, AimDirection>(m_context.currentPlayer);
        addWeaponAim(m_context.currentPlayer, pos.value, aim.value);
    }

    for (auto [weaponEntity, weaponParent, pos, aim] : m_context.registry.view<WeaponParent, Position, AimDirection>().each()) {
        if (weaponParent.parent != m_context.currentPlayer) {
            continue;
        }
        addWeaponAim(weaponEntity, pos.value, aim.value);
    }
    Vector2 posSum = {0, 0};
    for (auto aimLoc : aimLocs) {
        posSum += aimLoc;
        DrawCircleLines(aimLoc.x, aimLoc.y, 10, ColorAlpha(SKYBLUE, 0.15));
    }
    if (!aimLocs.empty()) {
        DrawCircleLines(posSum.x / aimLocs.size(), posSum.y / aimLocs.size(), 30, SKYBLUE);
    }
}

void BattlefieldHUDRenderer::drawAmmoCircle()
{
    if (!m_context.registry.valid(m_context.currentPlayer))
        return;

    const int ammoTextSize = 20;

    std::vector<std::tuple<float, float, int>> weaponAmmo;

    if (auto ammoPtr = m_context.registry.try_get<Ammo>(m_context.currentPlayer)) {
        auto reloadPtr = m_context.registry.try_get<AmmoReload>(m_context.currentPlayer);
        if (reloadPtr && reloadPtr->timer < reloadPtr->cd)
            weaponAmmo.push_back({reloadPtr->timer, reloadPtr->cd, true});
        else
            weaponAmmo.push_back({ammoPtr->value, ammoPtr->maxValue, false});
    } else if (auto cooldownPtr = m_context.registry.try_get<WeaponCooldown>(m_context.currentPlayer)) {
        weaponAmmo.push_back({std::min(1.0f, cooldownPtr->timeSinceLastShot / cooldownPtr->shootCooldown), 1.0, false});
    }

    for (auto [weaponEntity, weaponParent, ammo] : m_context.registry.view<WeaponParent, Ammo>().each()) {
        if (weaponParent.parent != m_context.currentPlayer) {
            continue;
        }
        auto reloadPtr = m_context.registry.try_get<AmmoReload>(weaponEntity);
        if (reloadPtr && reloadPtr->timer < reloadPtr->cd)
            weaponAmmo.push_back({reloadPtr->timer, reloadPtr->cd, true});
        else
            weaponAmmo.push_back({ammo.value, ammo.maxValue, false});
    }

    for (auto [weaponEntity, weaponParent, cooldown] : m_context.registry.view<WeaponParent, WeaponCooldown>(entt::exclude<Ammo>).each()) {
        if (weaponParent.parent == m_context.currentPlayer) {
            weaponAmmo.push_back({std::min(1.0f, cooldown.timeSinceLastShot / cooldown.shootCooldown), 1.0, false});
        }
    }

    if (weaponAmmo.empty())
        return;

    if (weaponAmmo.size() > 8) {
        weaponAmmo.resize(8);
    }

    Vector2 frameCenter = getUIFrameCenter();
    float frameRadius = getUIFrameRadius();
    float circleRadius = ammoTextSize * 0.75f;

    std::vector<Vector2> positions;

    int weaponsPerSide = (weaponAmmo.size() + 1) / 2;
    int leftSideWeapons = weaponsPerSide;
    int rightSideWeapons = weaponAmmo.size() - leftSideWeapons;

    float leftStartAngle = 240.0f - 90.0f;
    float leftGapAngle = 10.0f;

    for (int i = 0; i < leftSideWeapons; i++) {
        float angle = leftStartAngle + (leftGapAngle * i);
        float angleRad = angle * DEG2RAD;
        float distance = frameRadius + 60;

        Vector2 pos = {
            frameCenter.x + cosf(angleRad) * distance,
            frameCenter.y + sinf(angleRad) * distance
        };
        positions.push_back(pos);
    }

    std::reverse(positions.begin(), positions.end());

    float rightStartAngle = 120.0f - 90.0f;
    float rightGapAngle = 10.0f;

    for (int i = 0; i < rightSideWeapons; i++) {
        float angle = rightStartAngle - (rightGapAngle * i);
        float angleRad = angle * DEG2RAD;
        float distance = frameRadius + 60;

        Vector2 pos = {
            frameCenter.x + cosf(angleRad) * distance,
            frameCenter.y + sinf(angleRad) * distance
        };
        positions.push_back(pos);
    }

    m_reloadAngleOffset += 300.0f * m_currentDt;
    if (m_reloadAngleOffset >= 360.0f)
        m_reloadAngleOffset = 0;

    for (size_t i = 0; i < weaponAmmo.size() && i < positions.size(); i++) {
        Vector2 circleCenter = positions[i];
        auto [currentAmmo, maxAmmo, isReload] = weaponAmmo[i];
        float ammoRatio = maxAmmo > 0 ? currentAmmo / maxAmmo : 0.0f;

        DrawRingLines(circleCenter, circleRadius - 2, circleRadius + 2, 0, 360, 24, ColorAlpha(DARKGRAY, 0.8f));
        DrawRingLines(circleCenter, circleRadius - 1, circleRadius + 1, 0, 360, 24, ColorAlpha(BLACK, 0.6f));

        float startAngle = 0 - 90.0f;
        float endAngle = startAngle + (360 * ammoRatio);
        Color ammoColor = isReload ? SKYBLUE : (ammoRatio > 0.3f ? SKYBLUE : (ammoRatio > 0.1f ? YELLOW : RED));

        if (ammoRatio > 0) {
            if (!isReload)
                DrawRingLines(circleCenter, circleRadius - 2, circleRadius + 2, startAngle, endAngle, 32, ammoColor);
            else {
                const int segments = 4;
                for (int seg = 0; seg < segments; seg++) {
                    float angle = seg * 360.0f / segments + m_reloadAngleOffset;
                    DrawRingLines(circleCenter, circleRadius, circleRadius, angle, angle + 360.0f / segments / 2, 2, BLUE);
                }
            }
        }

        char ammoText[4];
        snprintf(ammoText, sizeof(ammoText), "%i", (int)currentAmmo + isReload);
        int textWidth = MeasureText(ammoText, ammoTextSize);
        DrawText(ammoText, circleCenter.x - textWidth/2, circleCenter.y - 7, ammoTextSize, WHITE);

        char weaponNum[4];
        snprintf(weaponNum, sizeof(weaponNum), "%zu", i + 1);
        int numWidth = MeasureText(weaponNum, 10);
        DrawText(weaponNum, circleCenter.x - numWidth/2, circleCenter.y - circleRadius - 15, 10, LIGHTGRAY);
    }
}

void BattlefieldHUDRenderer::drawCrosshair()
{
    Vector2 center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    float size = 15;
    float gap = 5;
    float thickness = 2;

    DrawRectangle(center.x - size - gap, center.y - thickness/2, size, thickness, WHITE);
    DrawRectangle(center.x + gap, center.y - thickness/2, size, thickness, WHITE);

    DrawRectangle(center.x - thickness/2, center.y - size - gap, thickness, size, WHITE);
    DrawRectangle(center.x - thickness/2, center.y + gap, thickness, size, WHITE);
}

void BattlefieldHUDRenderer::drawMainUIFrame()
{
    Vector2 center = getUIFrameCenter();
    float radius = getUIFrameRadius();
    float startAngle = 45.0f - 90.0f;
    float endAngle = 315.0f - 90.0f;

    DrawRingLines(center, radius, radius + 3, startAngle, endAngle, 64, ColorAlpha(SKYBLUE, 0.8f));

    for (int i = 0; i <= 10; i++) {
        float tickAngle = startAngle + (endAngle - startAngle) * i / 10.0f;
        float tickRadius1 = radius - 5;
        float tickRadius2 = (i % 2 == 0) ? radius - 10 : radius - 15;

        float angleRad = tickAngle * DEG2RAD;
        Vector2 tickStart = {center.x + cosf(angleRad) * tickRadius1, center.y + sinf(angleRad) * tickRadius1};
        Vector2 tickEnd = {center.x + cosf(angleRad) * tickRadius2, center.y + sinf(angleRad) * tickRadius2};

        DrawLineEx(tickStart, tickEnd, 2.0f, ColorAlpha(SKYBLUE, 0.7f));
    }
}

void BattlefieldHUDRenderer::drawCursorArrow()
{
    Vector2 mousePos = GetMousePosition();
    Vector2 screenCenter = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    Vector2 direction = Vector2Subtract(mousePos, screenCenter);
    float distance = Vector2Length(direction);

    if (distance > 0) {
        Vector2 normalizedDir = Vector2Normalize(direction);

        m_speedAnimationTime += m_currentDt * 1.0f;

        float arrowSpacing = 40.0f;
        float arrowSpeed = 200.0f;
        int numArrows = (int)(distance / arrowSpacing) + 1;

        for (int i = 0; i < numArrows; i++) {
            float baseOffset = i * arrowSpacing;
            float animOffset = fmod(m_speedAnimationTime * arrowSpeed, arrowSpacing);
            float totalOffset = baseOffset + animOffset;

            if (totalOffset >= distance) continue;

            Vector2 arrowPos = Vector2Add(screenCenter,
                Vector2Scale(normalizedDir, totalOffset));

            float progress = totalOffset / distance;
            float alpha = 1.0f - (progress * 0.3f);
            float arrowSize = 8.0f * (1.0f - progress * 0.2f);

            Vector2 arrowTip = Vector2Add(arrowPos, Vector2Scale(normalizedDir, arrowSize));
            Vector2 arrowLeft = Vector2Add(arrowPos, Vector2Scale(Vector2Rotate(normalizedDir, -2.5f), arrowSize * 0.6f));
            Vector2 arrowRight = Vector2Add(arrowPos, Vector2Scale(Vector2Rotate(normalizedDir, 2.5f), arrowSize * 0.6f));

            Color arrowColor = ColorAlpha(SKYBLUE, alpha * 0.9f);
            Color arrowBorder = ColorAlpha(DARKBLUE, alpha * 0.7f);

            DrawTriangle(Vector2Add(arrowTip, {1, 1}),
                        Vector2Add(arrowLeft, {1, 1}),
                        Vector2Add(arrowRight, {1, 1}),
                        arrowBorder);

            DrawTriangle(arrowTip, arrowLeft, arrowRight, arrowColor);
        }
    }

    float lineThickness = 1.0f;
    Color lineColor = ColorAlpha(SKYBLUE, 0.3f);
    DrawLineEx(screenCenter, mousePos, lineThickness, lineColor);

    float circleRadius = 8.0f;
    Color circleColor = WHITE;
    Color circleBorder = ColorAlpha(BLACK, 0.8f);

    DrawCircleV(mousePos, circleRadius + 1, circleBorder);
    DrawCircleV(mousePos, circleRadius, circleColor);
    DrawCircleV(mousePos, 2.0f, SKYBLUE);

    DrawCircleV(screenCenter, 4.0f, ColorAlpha(SKYBLUE, 0.8f));
    DrawCircleV(screenCenter, 2.0f, WHITE);
}

void BattlefieldHUDRenderer::drawCollisionWarning()
{
    if (!m_context.hudManager.hasCollisionWarnings())
        return;

    const auto &warnings = m_context.hudManager.getCollisionWarnings();
    float alpha = m_context.hudManager.getCollisionAlertAlpha();

    Vector2 screenCenter = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    float uiFrameRadius = getUIFrameRadius();

    for (const auto& [warningPos, distance]: warnings) {
        Vector2 screenPos = GetWorldToScreen(warningPos, m_camera);
        bool isOnScreen = (screenPos.x >= 0 && screenPos.x <= GetScreenWidth() &&
                           screenPos.y >= 0 && screenPos.y <= GetScreenHeight() &&
                           draw_utils::isInFrontOfCamera(warningPos, m_camera));

        if (isOnScreen) {
            float pulseRadius = 20.0f + 10.0f * (alpha - 0.7f) * 3.33f;
            DrawCircleLines(screenPos.x, screenPos.y, pulseRadius, ColorAlpha(RED, alpha));
            DrawCircleLines(screenPos.x, screenPos.y, pulseRadius + 2, ColorAlpha(YELLOW, alpha * 0.7f));

            DrawText("!", screenPos.x - 4, screenPos.y - 10, 20, ColorAlpha(RED, alpha));
        } else {
            Vector3 toWarning = warningPos - m_camera.position;
            Vector3 camForward = Vector3Normalize(m_camera.target - m_camera.position);
            Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, m_camera.up));
            Vector3 camUp = Vector3CrossProduct(camRight, camForward) * -1;

            Vector3 local;
            local.x = Vector3DotProduct(toWarning, camRight);
            local.y = Vector3DotProduct(toWarning, camUp);
            local.z = Vector3DotProduct(toWarning, camForward);

            Vector2 directionToWarning = {local.x, local.y};
            directionToWarning = Vector2Normalize(directionToWarning);

            char distText[32];
            snprintf(distText, sizeof(distText), "%.0fm", distance);
            int textWidth = MeasureText(distText, 20);
            Vector2 textPos = screenCenter + directionToWarning * (uiFrameRadius + 50);
            DrawText(distText, textPos.x - textWidth/2, textPos.y - 8, 20, ColorAlpha(RED, alpha));
        }
    }
}

namespace {
	Vector2 getTrianglePoint(Vector2 center, float radius, float angle) {
		float angleRad = angle * DEG2RAD;
		return Vector2{
			center.x + cosf(angleRad) * radius,
			center.y + sinf(angleRad) * radius
		};
	}

	void drawEquiTriangle(Vector2 center, float radius, float angle, Color color) {
		DrawTriangleLines(
			getTrianglePoint(center, radius, angle),
			getTrianglePoint(center, radius, angle + 120.0f),
			getTrianglePoint(center, radius, angle + 240.0f),
			color
		);
	}

	[[maybe_unused]] void drawEquiTriangleCorners(Vector2 center, float radius, float angle, Color color, float cornerRad) {
		for (int theta = 0; theta < 360; theta += 120) {
			drawEquiTriangle(getTrianglePoint(center, radius, angle + theta), cornerRad, angle + theta, color);
		}
	}
}

void BattlefieldHUDRenderer::drawMissileWarning()
{
    if (!m_context.hudManager.hasMissileWarnings())
        return;

    const auto &warnings = m_context.hudManager.getMissileWarnings();
    float alpha = m_context.hudManager.getMissileAlertAlpha();
	Color color = ColorLerp(ORANGE, MAROON, 0.5f);

    Vector2 screenCenter = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    float uiFrameRadius = getUIFrameRadius();

    for (const auto& [warningPos, distance]: warnings) {
        Vector2 screenPos = GetWorldToScreen(warningPos, m_camera);
        bool isOnScreen = (screenPos.x >= 0 && screenPos.x <= GetScreenWidth() &&
                           screenPos.y >= 0 && screenPos.y <= GetScreenHeight() &&
                           draw_utils::isInFrontOfCamera(warningPos, m_camera));

        if (isOnScreen) {
			Color aimColor = ColorAlpha(color, alpha);
            drawEquiTriangle(screenPos, 15, -90, ColorAlpha(color, alpha * 0.5f));
            drawEquiTriangle(screenPos, 15 + 4, -90, aimColor);

            DrawText("!", screenPos.x - 1, screenPos.y - 10, 20, ColorAlpha(color, alpha));
        } else {
            Vector3 toWarning = warningPos - m_camera.position;
            Vector3 camForward = Vector3Normalize(m_camera.target - m_camera.position);
            Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, m_camera.up));
            Vector3 camUp = Vector3CrossProduct(camRight, camForward) * -1;

            Vector3 local;
            local.x = Vector3DotProduct(toWarning, camRight);
            local.y = Vector3DotProduct(toWarning, camUp);
            local.z = Vector3DotProduct(toWarning, camForward);

            Vector2 directionToWarning = {local.x, local.y};
            directionToWarning = Vector2Normalize(directionToWarning);

            char distText[32];
            snprintf(distText, sizeof(distText), "%.0fm", distance);
            int textWidth = MeasureText(distText, 20);
            Vector2 textPos = screenCenter + directionToWarning * (uiFrameRadius + 50);
            DrawText(distText, textPos.x - textWidth / 2, textPos.y - 8, 20, ColorAlpha(color, alpha));
        }
    }
}

void BattlefieldHUDRenderer::drawDamageNumbers(const Camera3D &camera)
{
    if (!m_context.config.settings.showDamageNumbers)
        return;

    for (const auto &d : m_context.hudManager.getActiveDamageNumbers()) {
        if (!draw_utils::isInFrontOfCamera(d.worldPos, camera))
            continue;

        Vector2 screen = GetWorldToScreen(d.worldPos, camera);
        if (screen.x < -100 || screen.x > GetScreenWidth() + 100 ||
            screen.y < -100 || screen.y > GetScreenHeight() + 100)
            continue;

        float progress = d.timer / d.maxDuration;
        float alpha = (progress > 0.7f) ? (1.0f - (progress - 0.7f) / 0.3f) : 1.0f;
        alpha *= m_context.config.hud.damageNumbers.opacity;
        Color baseColor = hitTypeToColor(d.hitType);
        Color drawColor = ColorAlpha(baseColor, alpha);

        int dmgInt = static_cast<int>(std::round(d.totalDamage));
        const char *txt = TextFormat("%d", dmgInt);
        int sz = static_cast<int>(m_context.config.hud.damageNumbers.fontSize * d.scale);
        int w = MeasureText(txt, sz);

        DrawText(txt, static_cast<int>(screen.x) - w / 2, static_cast<int>(screen.y) - 24 - sz / 2, sz, drawColor);
    }
}

void BattlefieldHUDRenderer::drawWarningToastItem(const HudManager::ActiveToast &t, Vector2 slotPos, float alpha)
{
    const auto &cfg = m_context.config.hud.warningToasts;
    const int fontSize = cfg.fontSize;
    const int msgWidth = MeasureText(t.text.c_str(), fontSize);
    const float x = slotPos.x - msgWidth / 2.0f;
    const float y = slotPos.y - 12.0f;

    const float pulseAlpha = (t.color.a > 0) ? (t.color.a / 255.0f) * alpha : alpha;
    const float halfPadX = cfg.boxPaddingX / 2.0f;

    DrawRectangle(static_cast<int>(x - halfPadX), static_cast<int>(y - 5), msgWidth + cfg.boxPaddingX, cfg.boxHeight, ColorAlpha(RED, pulseAlpha * cfg.fillOpacity));
    DrawRectangleLines(static_cast<int>(x - halfPadX), static_cast<int>(y - 5), msgWidth + cfg.boxPaddingX, cfg.boxHeight, ColorAlpha(RED, pulseAlpha));
    DrawText(t.text.c_str(), static_cast<int>(x), static_cast<int>(y), fontSize, ColorAlpha(RED, pulseAlpha));
}

void BattlefieldHUDRenderer::drawToasts()
{
    int leftLogIndex = 0;
    float leftBaseY = static_cast<float>(GetScreenHeight()) - 180.0f;

    for (const auto &t : m_context.hudManager.getActiveToasts()) {
        float progress = (t.maxDuration > 0.0f) ? (t.timer / t.maxDuration) : 0.0f;
        float alpha = (progress > 0.8f) ? (1.0f - (progress - 0.8f) / 0.2f) : 1.0f;
        Color col = ColorAlpha(t.color, alpha);

        if (t.slot == ToastSlot::LEFT_LOG) {
            if (!m_context.config.settings.showKillLogs)
                continue;
            float yPos = leftBaseY - (leftLogIndex * (t.fontSize + 8.0f));
            DrawRectangle(15, static_cast<int>(yPos - 2), MeasureText(t.text.c_str(), t.fontSize) + 12, t.fontSize + 4, ColorAlpha(BLACK, 0.4f * alpha));
            DrawText(t.text.c_str(), 20, static_cast<int>(yPos), t.fontSize, col);
            leftLogIndex++;
        } else if (t.slot == ToastSlot::TOP_NOTIF) {
            if (!m_context.config.settings.showToasts)
                continue;
            int w = MeasureText(t.text.c_str(), t.fontSize);
            float xPos = (GetScreenWidth() - w) * 0.5f;
            float yPos = 80.0f;
            DrawRectangle(static_cast<int>(xPos - 16), static_cast<int>(yPos - 4), w + 32, t.fontSize + 8, ColorAlpha(BLACK, 0.6f * alpha));
            DrawRectangleLines(static_cast<int>(xPos - 16), static_cast<int>(yPos - 4), w + 32, t.fontSize + 8, ColorAlpha(t.color, 0.8f * alpha));
            DrawText(t.text.c_str(), static_cast<int>(xPos), static_cast<int>(yPos), t.fontSize, col);
        } else if (t.slot >= ToastSlot::WARNING_TOP) {
            if (!m_context.config.settings.showToasts)
                continue;
            const Vector2 center = getUIFrameCenter();
            const float radius = getUIFrameRadius();
            const float offsetY = m_context.config.hud.warningToasts.slotOffsetY;
            Vector2 slotPos{center.x, center.y - radius - 35.0f};
            if (t.slot == ToastSlot::WARNING_LEFT)
                slotPos = {center.x - radius * 0.85f, center.y - radius - offsetY};
            else if (t.slot == ToastSlot::WARNING_RIGHT)
                slotPos = {center.x + radius * 0.85f, center.y - radius - offsetY};
            drawWarningToastItem(t, slotPos, alpha);
        } else {
            if (!m_context.config.settings.showToasts)
                continue;
            DrawText(t.text.c_str(), static_cast<int>(t.screenPos.x), static_cast<int>(t.screenPos.y), t.fontSize, col);
        }
    }
}

Vector2 BattlefieldHUDRenderer::getUIFrameCenter() const
{
    return {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
}

float BattlefieldHUDRenderer::getUIFrameRadius() const
{
    return fminf(GetScreenWidth(), GetScreenHeight()) * 0.25f;
}
