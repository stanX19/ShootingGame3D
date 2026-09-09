#include "game.hpp"
#include "engine.hpp" // For EngineState
#include "shoot_3d.hpp"

Game::Game(GameContext &context) 
    : context(context), 
      renderer(context.mainCamera, context), 
      hudRenderer(context.mainCamera, context) 
{}

Game::~Game() {}

void Game::reset() {
    context.registry.clear();
    weapon::utils::setUpRegistry(context);
    event::utils::hookAllListeners(context);
    spawnPlayer(context);
    spawnSunAndStars(context);
    SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    context.factions.clear();
    
    context.mainCamera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    context.mainCamera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    context.mainCamera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    context.mainCamera.fovy = 45.0f;
    context.mainCamera.projection = CAMERA_PERSPECTIVE;

    context.soundManager.playImmediate(context.config, "sounds.gameStart", 0.25f);
}

EngineState Game::run() {
    context.soundManager.playMusic();
    EngineState nextState = EngineState::GAME;

    while (!WindowShouldClose() && nextState == EngineState::GAME) {
        float dt = GetFrameTime();
        context.soundManager.updateMusic();

        // --- Update systems ---
        m_systemPlayerMoveControl.update(context, dt);
        m_systemPlayerRespawn.update(context, dt);
        m_systemAiFindTarget.update(context, dt);
        m_systemAiMoveControl.update(context, dt);
        m_systemAiShootControl.update(context, dt);
        m_systemProcessMoveRequest.update(context, dt);

        m_systemAmmoReload.update(context, dt);
        m_systemBulletTargetAim.update(context, dt);
        m_systemWeaponParentControlAim.update(context, dt);
        m_systemWeaponParentControlShoot.update(context, dt);
        m_systemPlayerShootControl.update(context, dt);  // override parent control shoot
        m_systemWeaponUpdateCooldown.update(context, dt);
        m_systemWeaponUpdateCanFire.update(context, dt);
        m_systemWeaponUpdateCharged.update(context, dt);
        m_systemWeaponShoot.update(context, dt);
        m_systemWeaponUpdateFireStatus.update(context, dt);

        m_systemEntityMovement.update(context, dt);
        m_systemEntityAnchor.update(context, dt);
        m_systemEntityTransformation.update(context, dt);
        
        m_systemDetectEntityCollision.update(context, dt);
        context.dispatcher.update();
        context.soundManager.update(context.mainCamera);
        m_systemSoundSfx.update(context, dt);

        m_systemEnergyShield.update(context, dt);
        
        m_systemSyncModelRotation.update(context, dt);
        m_systemCameraFollowPlayer.update(context, dt);
        BeginDrawing();
        renderer.Render(dt);
        hudRenderer.RenderAll(dt);
        EndDrawing();

        m_systemBlueUnitRespawn.update(context, dt);
        m_systemRedUnitRespawn.update(context, dt);
        m_systemAsteroidRespawn.update(context, dt);
        m_systemEntityAnchorRelease.update(context, dt);
        m_systemEntityLifetime.update(context, dt);
        m_systemCleanOutOfBound.update(context, dt);
        m_systemDelayedDamage.update(context, dt);
        m_systemHpCleanup.update(context, dt);
        m_systemHpRegen.update(context, dt);
        m_systemSpawnTrailParticles.update(context, dt);

        inputControls(dt, nextState);
    }

    context.soundManager.stopMusic();   
    return nextState;
}

void Game::inputControls([[maybe_unused]] float dt, EngineState &nextState) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        nextState = EngineState::MENU;
        return;
    }
    if (IsKeyPressed(KEY_R)) {
        reset();
    }
    if (IsKeyPressed(KEY_DELETE) && context.registry.valid(context.currentPlayer)) {
        context.registry.emplace<DelayedDamage>(context.currentPlayer, DelayedDamage{0.0f, 100000000.0f});
        spawnPlayer(context);
    }
    if (IsKeyPressed(KEY_C)) {
        SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    }
}
