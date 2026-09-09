#pragma once

#include "systems/base_system.hpp"
#include "components/weapon.hpp"

struct GameContext;

// utils
bool aimTargetExists(GameContext &context, AimTarget &target);

namespace systems
{
	class CameraFollowPlayer : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class PlayerMoveControl : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class PlayerShootControl : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class PlayerRespawn : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class AiMoveControl : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class AiShootControl : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class AiFindTarget : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class BlueUnitRespawn : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class RedUnitRespawn : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class ProcessMoveRequest : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class EntityMovement : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class EntityTransformation : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class DetectEntityCollision : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class EntityAnchor : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class EntityAnchorRelease : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class EntityLifetime : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class DelayedDamage : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class HpCleanup : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class HpRegen : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class EnergyShield : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class AmmoReload : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class BulletTargetAim : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class WeaponShoot : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class WeaponParentControlAim : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class WeaponParentControlShoot : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class WeaponUpdateCanFire : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class WeaponUpdateFireStatus : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class WeaponUpdateCooldown : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class WeaponUpdateCharged : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class AsteroidRespawn : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class CleanOutOfBound : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class SyncModelRotation : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class SpawnTrailParticles : public BaseSystem { public: void update(GameContext &context, float dt) override; };
	class SoundSfx : public BaseSystem { public: void update(GameContext &context, float dt) override; };
} // namespace systems