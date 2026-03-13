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
        ecs_systems::playerMoveControl(context, dt);
        ecs_systems::playerRespawn(context, dt);
        ecs_systems::aiFindTarget(context, dt);
        ecs_systems::aiMoveControl(context, dt);
        ecs_systems::aiShootControl(context, dt);
        ecs_systems::processMoveRequest(context, dt);

        ecs_systems::ammoReload(context, dt);
        ecs_systems::bulletTargetAim(context, dt);
        ecs_systems::weaponParentControlAim(context, dt);
        ecs_systems::weaponParentControlShoot(context, dt);
        ecs_systems::playerShootControl(context, dt);  // override parent control shoot
        ecs_systems::weaponUpdateCooldown(context, dt);
        ecs_systems::weaponUpdateCanFire(context, dt);
        ecs_systems::weaponUpdateCharged(context, dt);
        ecs_systems::weaponShoot(context, dt);
        ecs_systems::weaponUpdateFireStatus(context, dt);

        ecs_systems::entityMovement(context, dt);
        ecs_systems::entityAnchor(context, dt);
        ecs_systems::entityTransformation(context, dt);
        
        ecs_systems::detectEntityCollision(context, dt);
        context.dispatcher.update();
        context.soundManager.update(context.mainCamera);
        ecs_systems::soundSfx(context, dt);

        ecs_systems::energyShield(context, dt);
        
        ecs_systems::syncModelRotation(context, dt);
        ecs_systems::cameraFollowPlayer(context, dt);
        BeginDrawing();
        renderer.Render(dt);
        hudRenderer.RenderAll(dt);
        EndDrawing();

        ecs_systems::blueUnitRespawn(context, dt);
        ecs_systems::redUnitRespawn(context, dt);
        ecs_systems::asteroidRespawn(context, dt);
        ecs_systems::entityAnchorRelease(context, dt);
        ecs_systems::entityLifetime(context, dt);
        ecs_systems::cleanOutOfBound(context, dt);
        ecs_systems::delayedDamage(context, dt);
        ecs_systems::hpCleanup(context, dt);
        ecs_systems::hpRegen(context, dt);
        ecs_systems::spawnTrailParticles(context, dt);

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
