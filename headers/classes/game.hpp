#ifndef GAME_HPP
#define GAME_HPP

#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "battlefield_hud_renderer.hpp"
#include "systems.hpp"

enum class EngineState; // Forward declaration

class Game {
public:
    Game(GameContext &context);
    ~Game();

    void reset();
    EngineState run();

private:
    GameContext &m_context;
    Renderer m_renderer;
    BattlefieldHUDRenderer m_hudRenderer;

    // Systems
    systems::PlayerMoveControl m_systemPlayerMoveControl;
    systems::PlayerRespawn m_systemPlayerRespawn;
    systems::AiFindTarget m_systemAiFindTarget;
    systems::AiMoveControl m_systemAiMoveControl;
    systems::AiShootControl m_systemAiShootControl;
    systems::ProcessMoveRequest m_systemProcessMoveRequest;

    systems::AmmoReload m_systemAmmoReload;
    systems::BulletTargetAim m_systemBulletTargetAim;
    systems::WeaponParentControlAim m_systemWeaponParentControlAim;
    systems::WeaponParentControlShoot m_systemWeaponParentControlShoot;
    systems::PlayerShootControl m_systemPlayerShootControl;
    systems::WeaponUpdateCooldown m_systemWeaponUpdateCooldown;
    systems::WeaponUpdateCanFire m_systemWeaponUpdateCanFire;
    systems::WeaponUpdateCharged m_systemWeaponUpdateCharged;
    systems::WeaponShoot m_systemWeaponShoot;
    systems::WeaponUpdateFireStatus m_systemWeaponUpdateFireStatus;

    systems::EntityMovement m_systemEntityMovement;
    systems::EntityAnchor m_systemEntityAnchor;
    systems::EntityTransformation m_systemEntityTransformation;

    systems::DetectEntityCollision m_systemDetectEntityCollision;
    systems::SoundSfx m_systemSoundSfx;

    systems::EnergyShield m_systemEnergyShield;

    systems::SyncModelRotation m_systemSyncModelRotation;
    systems::CameraFollowPlayer m_systemCameraFollowPlayer;
    systems::HudWarning m_systemHudWarning;

    systems::BlueUnitRespawn m_systemBlueUnitRespawn;
    systems::RedUnitRespawn m_systemRedUnitRespawn;
    systems::AsteroidRespawn m_systemAsteroidRespawn;
    systems::EntityAnchorRelease m_systemEntityAnchorRelease;
    systems::EntityLifetime m_systemEntityLifetime;
    systems::CleanOutOfBound m_systemCleanOutOfBound;
    systems::DelayedDamage m_systemDelayedDamage;
    systems::HpCleanup m_systemHpCleanup;
    systems::HpRegen m_systemHpRegen;
    systems::SpawnTrailParticles m_systemSpawnTrailParticles;

    void inputControls([[maybe_unused]] float dt, EngineState &nextState);
};

#endif // GAME_HPP
