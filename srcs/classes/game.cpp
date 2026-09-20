#include "game.hpp"
#include "engine.hpp" // For EngineState
#include "shoot_3d.hpp"

Game::Game(GameContext &context) 
    : m_context(context), 
      m_renderer(context), 
      m_hudRenderer(context.mainCamera, context) 
{}

Game::~Game() {}

void Game::reset() {
    m_context.registry.clear();
    weapon::utils::setUpRegistry(m_context);
    event::utils::hookAllListeners(m_context);
    spawnPlayer(m_context);
    spawnSunAndStars(m_context);
    SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    m_context.factions.clear();
    m_context.hudManager.reset();
    
    m_context.mainCamera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    m_context.mainCamera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    m_context.mainCamera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    m_context.mainCamera.fovy = 45.0f;
    m_context.mainCamera.projection = CAMERA_PERSPECTIVE;

    m_context.soundManager.playImmediate(m_context.config, "sounds.gameStart", 0.25f);
}

EngineState Game::run() {
    m_context.soundManager.playMusic();
    EngineState nextState = EngineState::GAME;

    while (!WindowShouldClose() && nextState == EngineState::GAME) {
        float dt = GetFrameTime();
        m_context.gameTime += dt;
        m_context.soundManager.updateMusic();

        // --- Update systems ---
        m_systemPlayerMoveControl.update(m_context, dt);
        m_systemPlayerRespawn.update(m_context, dt);
        m_systemAiFindTarget.update(m_context, dt);
        m_systemAiMoveControl.update(m_context, dt);
        m_systemAiShootControl.update(m_context, dt);
        m_systemProcessMoveRequest.update(m_context, dt);

        m_systemAmmoReload.update(m_context, dt);
        m_systemBulletTargetAim.update(m_context, dt);
        m_systemWeaponParentControlAim.update(m_context, dt);
        m_systemWeaponParentControlShoot.update(m_context, dt);
        m_systemPlayerShootControl.update(m_context, dt);  // override parent control shoot
        m_systemWeaponUpdateCooldown.update(m_context, dt);
        m_systemWeaponUpdateCanFire.update(m_context, dt);
        m_systemWeaponUpdateCharged.update(m_context, dt);
        m_systemWeaponShoot.update(m_context, dt);
        m_systemWeaponUpdateFireStatus.update(m_context, dt);

        m_systemEntityMovement.update(m_context, dt);
        m_systemEntityAnchor.update(m_context, dt);
        m_systemEntityTransformation.update(m_context, dt);
        m_systemSyncModelRotation.update(m_context, dt);
        
        m_systemDetectEntityCollision.update(m_context, dt);
        m_context.dispatcher.update();
        m_context.soundManager.update(m_context.mainCamera);
        m_systemSoundSfx.update(m_context, dt);

        m_systemEnergyShield.update(m_context, dt);
        m_systemCameraFollowPlayer.update(m_context, dt);
        m_systemHudWarning.update(m_context, dt);
        m_context.hudManager.setObservedEntity(m_context.currentPlayer);
        m_context.hudManager.update(dt, m_context);
        BeginDrawing();
        m_renderer.render(dt, m_context.hudManager.getRenderingCamera());
        m_hudRenderer.renderAll(dt);
        EndDrawing();

        m_systemUnitSpawn.update(m_context, dt);
        m_systemAsteroidRespawn.update(m_context, dt);
        m_systemEntityAnchorRelease.update(m_context, dt);
        m_systemEntityLifetime.update(m_context, dt);
        m_systemCleanOutOfBound.update(m_context, dt);
        m_systemDelayedDamage.update(m_context, dt);
        m_systemHpCleanup.update(m_context, dt);
        m_systemHpRegen.update(m_context, dt);
        m_systemSpawnTrailParticles.update(m_context, dt);

        inputControls(dt, nextState);
    }

    m_context.soundManager.stopMusic();   
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
    if (IsKeyPressed(KEY_DELETE) && m_context.registry.valid(m_context.currentPlayer)) {
        m_context.registry.emplace<combat::DelayedDamage>(m_context.currentPlayer, combat::DelayedDamage{0.0f, 100000000.0f});
        spawnPlayer(m_context);
    }
    if (IsKeyPressed(KEY_C)) {
        SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    }
}
