#include "battlefield_hud_renderer.hpp"
#include "draw_utils.hpp"
#include <algorithm>
#include <tuple>
#include <vector>
#include <cmath>
#include <cstdio>

BattlefieldHUDRenderer::BattlefieldHUDRenderer(Camera3D &camera, GameContext &context)
    : camera(camera), context(context)
{
}

void BattlefieldHUDRenderer::Render(float dt)
{
    currentDt = dt;
    drawHUD();
    drawTexts();
}

void BattlefieldHUDRenderer::drawHUD()
{
    drawHealthBars();
    drawTargetable();

    if (!context.registry.valid(context.currentPlayer))
        return;

    drawAimCircle();
    drawMainUIFrame();
    drawSpeedBar();
    // drawThrustBar();
    drawAmmoCircle();
    drawCrosshair();
    drawCursorArrow();
    drawCollisionWarning();
}

void BattlefieldHUDRenderer::drawTexts()
{
    static int score = 0;

    DrawFPS(10, 10);

    if (context.registry.valid(context.currentPlayer))
    {
        Color textColor = SKYBLUE;
        int totalEntities = 0;
        auto hittableView = context.registry.view<CollisionBody, Position, HP>();
        for (auto entity : hittableView)
        {
            if (hittableView.get<HP>(entity).value > 0)
            {
                totalEntities++;
            }
        }
        DrawText(TextFormat("Entities: %d", totalEntities), 10, 30, 20, textColor);
        DrawText("Move: W S or Right click", 10, 50, 20, textColor);
        DrawText("Turn: Arrows or Mouse cursor", 10, 70, 20, textColor);
        DrawText("Fire: Space or Left click", 10, 90, 20, textColor);

        char buf[40];
        Score *scorePtr = context.registry.try_get<Score>(context.currentPlayer);
        score = scorePtr ? scorePtr->value : -1;
        sprintf(buf, "score: %i", score);
        DrawText(buf, 10, 130, 20, textColor);
    }
    else
    {
        const char *msg = "GAME OVER - PRESS R TO RESTART";
        int w = MeasureText(msg, 40);
        DrawText(msg, GetScreenWidth() / 2 - w / 2, GetScreenHeight() / 2 + 50, 40, RED);

        char buf[40];
        sprintf(buf, "Final Score: %i", score);
        w = MeasureText(buf, 50);
        DrawText(buf, GetScreenWidth() / 2 - w / 2, GetScreenHeight() / 2, 50, ORANGE);
    }
}

void BattlefieldHUDRenderer::drawHealthBars()
{
    auto view = context.registry.view<Position, CollisionBody, HP, tag::Targetable>();
    for (auto entity : view)
    {
        auto &pos = view.get<Position>(entity);
        auto &hp = view.get<HP>(entity);
        EnergyShield *shieldPtr = context.registry.try_get<EnergyShield>(entity);

        if (hp.value == hp.maxValue && (!shieldPtr || shieldPtr->hp == shieldPtr->maxHp))
            continue;
        if (!draw_utils::isInFrontOfCamera(pos.value, camera))
            continue;
        Vector2 screen = GetWorldToScreen(pos.value, camera);

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
    auto [aimTargetPtr, playerPosPtr, playerFacPtr] = context.registry.try_get<AimTarget, Position, faction::Faction>(context.currentPlayer);
    entt::entity targetedEntity = aimTargetPtr ? aimTargetPtr->entity : entt::null;
    Vector3 playerPos = playerPosPtr ? playerPosPtr->value : camera.target;
    faction::FacVal playerFac = playerFacPtr ? playerFacPtr->value : 0;

    Vector3 camForward = Vector3Normalize(camera.target - camera.position);
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, camera.up));
    Vector3 camUp = Vector3CrossProduct(camRight, camForward);

    Vector2 screenCenter = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    float uiFrameRadius = GetUIFrameRadius();

    static float animationAngle = 0.0f;
    animationAngle = wrapAngleDegree(animationAngle + 1.0f);

    static entt::entity s_prevTargetedEntity = entt::null;
    static float s_blinkTimer = 0.0f;
    const float blinkInterval = 0.1f;

    s_blinkTimer -= currentDt;
    if (targetedEntity != s_prevTargetedEntity) {
        s_blinkTimer = blinkInterval * 3;
    }
    s_prevTargetedEntity = targetedEntity;
    std::vector<std::tuple<entt::entity, Vector3, float, faction::FacVal>> allPosArr;
    std::vector<std::tuple<entt::entity, Vector3, float, faction::FacVal>> posArr;
    for (auto [entity, pos, faction] : context.registry.view<Position, tag::Targetable, faction::Faction>().each())
    {
        if (entity == context.currentPlayer)
            continue;
        float dist = Vector3Distance(pos.value, playerPos);
        faction::FacVal isAlly = faction.value & playerFac;
        if (dist < COMBAT_DIST * 2.5f)
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

        Vector2 screenPos = GetWorldToScreen(pos, camera);

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

            if (s_blinkTimer >= 0 && fmod(s_blinkTimer / blinkInterval, 2.0f) >= 1.0f) {
                aimColor = ColorAlpha(aimColor, 0.3f);
            }

            DrawRingLines(screenPos, innerRad, innerRad + 2, 90 + animationAngle, 180 + animationAngle, 12, aimColor);
            DrawRingLines(screenPos, innerRad, innerRad + 2, 270 + animationAngle, 360 + animationAngle, 12, aimColor);
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
    if (!context.registry.all_of<Velocity, MaxSpeed, Rotation>(context.currentPlayer))
        return;

    entt::entity entity = context.currentPlayer;
    const auto& velocity = context.registry.get<Velocity>(entity);
    const auto& maxSpeed = context.registry.get<MaxSpeed>(entity);
    const auto& rotation = context.registry.get<Rotation>(entity);

    float currentSpeed = Vector3DotProduct(velocity.value, getForwardVector(rotation));
    float speedRatio = currentSpeed / (maxSpeed.value * 2);
    speedRatio = std::min(1.0, speedRatio > 0.5 ? 0.8 + 0.2 * ((speedRatio - 0.5) / 0.5) : speedRatio / 0.5 * 0.8);

    Vector2 center = GetUIFrameCenter();
    float frameRadius = GetUIFrameRadius();

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
    if (!context.registry.all_of<Velocity>(context.currentPlayer))
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
    if (!context.registry.valid(context.currentPlayer))
        return;

    std::vector<Vector2> aimLocs;

    auto addWeaponAim = [&](entt::entity entity, Vector3 pos, Vector3 aim) {
        float dist = COMBAT_DIST;
        auto targetPtr = context.registry.try_get<AimTarget>(entity);
        if (targetPtr) {
            auto targetPosPtr = context.registry.try_get<Position>(targetPtr->entity);
            if (targetPosPtr)
                dist = Vector3Distance(pos, targetPosPtr->value);
        }
        Vector3 position = pos + aim * dist;
        aimLocs.push_back(GetWorldToScreen(position, camera));
    };

    if (context.registry.all_of<Position, AimDirection>(context.currentPlayer)) {
        auto [pos, aim] = context.registry.get<Position, AimDirection>(context.currentPlayer);
        addWeaponAim(context.currentPlayer, pos.value, aim.value);
    }

    for (auto [weaponEntity, weaponParent, pos, aim] : context.registry.view<WeaponParent, Position, AimDirection>().each()) {
        if (weaponParent.parent != context.currentPlayer) {
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
    if (!context.registry.valid(context.currentPlayer))
        return;

    const int ammoTextSize = 20;

    std::vector<std::tuple<float, float, int>> weaponAmmo;

    if (auto ammoPtr = context.registry.try_get<Ammo>(context.currentPlayer)) {
        auto reloadPtr = context.registry.try_get<AmmoReload>(context.currentPlayer);
        if (reloadPtr && reloadPtr->timer < reloadPtr->cd)
            weaponAmmo.push_back({reloadPtr->timer, reloadPtr->cd, true});
        else
            weaponAmmo.push_back({ammoPtr->value, ammoPtr->maxValue, false});
    } else if (auto cooldownPtr = context.registry.try_get<WeaponCooldown>(context.currentPlayer)) {
        weaponAmmo.push_back({std::min(1.0f, cooldownPtr->timeSinceLastShot / cooldownPtr->shootCooldown), 1.0, false});
    }

    for (auto [weaponEntity, weaponParent, ammo] : context.registry.view<WeaponParent, Ammo>().each()) {
        if (weaponParent.parent != context.currentPlayer) {
            continue;
        }
        auto reloadPtr = context.registry.try_get<AmmoReload>(weaponEntity);
        if (reloadPtr && reloadPtr->timer < reloadPtr->cd)
            weaponAmmo.push_back({reloadPtr->timer, reloadPtr->cd, true});
        else
            weaponAmmo.push_back({ammo.value, ammo.maxValue, false});
    }

    for (auto [weaponEntity, weaponParent, cooldown] : context.registry.view<WeaponParent, WeaponCooldown>(entt::exclude<Ammo>).each()) {
        if (weaponParent.parent == context.currentPlayer) {
            weaponAmmo.push_back({std::min(1.0f, cooldown.timeSinceLastShot / cooldown.shootCooldown), 1.0, false});
        }
    }

    if (weaponAmmo.empty())
        return;

    if (weaponAmmo.size() > 8) {
        weaponAmmo.resize(8);
    }

    Vector2 frameCenter = GetUIFrameCenter();
    float frameRadius = GetUIFrameRadius();
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

    static float reloadAngleOffset = 0;
    reloadAngleOffset += 300.0f * currentDt;
    if (reloadAngleOffset >= 360.0f)
        reloadAngleOffset = 0;

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
                for (int i = 0; i < segments; i++) {
                    float angle = i * 360.0f / segments + reloadAngleOffset;
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
    Vector2 center = GetUIFrameCenter();
    float radius = GetUIFrameRadius();
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

        static float animationTime = 0.0f;
        animationTime += currentDt * 1.0f;

        float arrowSpacing = 40.0f;
        float arrowSpeed = 200.0f;
        int numArrows = (int)(distance / arrowSpacing) + 1;

        for (int i = 0; i < numArrows; i++) {
            float baseOffset = i * arrowSpacing;
            float animOffset = fmod(animationTime * arrowSpeed, arrowSpacing);
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
    const static float warningTime = 5.0f;
    const static float warningDist = 10.0f;
    std::vector<Vector3> warnings;
    auto [posA, velA, bodyA] = context.registry.try_get<Position, Velocity, CollisionBody>(context.currentPlayer);

    if (!posA || !velA || !bodyA)
        return;
    for (auto [other, posB, velB, bodyB, dmgB] : context.registry.view<Position, Velocity, CollisionBody, Damage, tag::Asteroid>(entt::exclude<tag::Bullet>).each()) {
        if (context.currentPlayer == other)
            continue;

        if (willCollide(posA->value, velA->value, posB.value, velB.value, bodyA->radius + bodyB.radius + warningDist, warningTime)) {
            warnings.push_back(posB.value);
        }
    }
    for (auto [other, posB, bodyB, dmgB] : context.registry.view<Position, CollisionBody, Damage, tag::Asteroid>(entt::exclude<tag::Bullet, Velocity>).each()) {
        if (context.currentPlayer == other)
            continue;

        if (willCollide(posA->value, velA->value, posB.value, Vector3Zero(), bodyA->radius + bodyB.radius + warningDist, warningTime)) {
            warnings.push_back(posB.value);
        }
    }

    if (warnings.empty())
        return;

    const char* alertMsg = "PROXIMITY ALERT";
    int msgWidth = MeasureText(alertMsg, 24);
    Vector2 alertPos = {GetScreenWidth() / 2.0f - msgWidth / 2.0f, 50.0f};

    static float blinkTimer = 0.0f;
    blinkTimer += currentDt * 6.0f;
    float alpha = 0.7f + 0.3f * sinf(blinkTimer);

    DrawRectangle(alertPos.x - 10, alertPos.y - 5, msgWidth + 20, 34, ColorAlpha(RED, alpha * 0.3f));
    DrawRectangleLines(alertPos.x - 10, alertPos.y - 5, msgWidth + 20, 34, ColorAlpha(RED, alpha));

    DrawText(alertMsg, alertPos.x, alertPos.y, 24, ColorAlpha(RED, alpha));

    Vector2 screenCenter = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    float uiFrameRadius = GetUIFrameRadius();

    for (const Vector3& warningPos : warnings) {
        Vector2 screenPos = GetWorldToScreen(warningPos, camera);
        bool isOnScreen = (screenPos.x >= 0 && screenPos.x <= GetScreenWidth() &&
                           screenPos.y >= 0 && screenPos.y <= GetScreenHeight() &&
                           draw_utils::isInFrontOfCamera(warningPos, camera));

        if (isOnScreen) {
            float pulseRadius = 20.0f + 10.0f * sinf(blinkTimer * 2.0f);
            DrawCircleLines(screenPos.x, screenPos.y, pulseRadius, ColorAlpha(RED, alpha));
            DrawCircleLines(screenPos.x, screenPos.y, pulseRadius + 2, ColorAlpha(YELLOW, alpha * 0.7f));

            DrawText("!", screenPos.x - 4, screenPos.y - 10, 20, ColorAlpha(RED, alpha));
        } else {
            Vector3 toWarning = warningPos - camera.position;
            Vector3 camForward = Vector3Normalize(camera.target - camera.position);
            Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, camera.up));
            Vector3 camUp = Vector3CrossProduct(camRight, camForward);

            Vector3 local;
            local.x = Vector3DotProduct(toWarning, camRight);
            local.y = Vector3DotProduct(toWarning, camUp);
            local.z = Vector3DotProduct(toWarning, camForward);

            Vector2 directionToWarning = {local.x, local.y};

            directionToWarning = Vector2Normalize(directionToWarning);

            float distance = Vector3Length(toWarning);
            char distText[32];
            snprintf(distText, sizeof(distText), "%.0fm", distance);
            int textWidth = MeasureText(distText, 16);
            Vector2 textPos = screenCenter + directionToWarning * (uiFrameRadius + 50);
            DrawText(distText, textPos.x - textWidth/2, textPos.y - 8, 16, ColorAlpha(RED, alpha));
        }
    }
}

Vector2 BattlefieldHUDRenderer::GetUIFrameCenter() const
{
    return {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
}

float BattlefieldHUDRenderer::GetUIFrameRadius() const
{
    return fminf(GetScreenWidth(), GetScreenHeight()) * 0.25f;
}
