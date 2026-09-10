#pragma once

#include "systems/base_system.hpp"
#include "components/weapon.hpp"
#include "components/unit_camera.hpp"
#include <random>

struct GameContext;

// utils
bool aimTargetExists(const GameContext &context, const AimTarget &target);

namespace systems
{
	class CameraFollowPlayer : public BaseSystem
	{
	public:
		void update(GameContext &context, float dt) override;
	private:
		camera::UnitCamera m_defaultCamera{};
	};

	class PlayerMoveControl : public BaseSystem
	{
	public:
		void update(GameContext &context, float dt) override;
	private:
		float m_boostCooldown = 0.0f;
		float m_boostDuration = 0.0f;
	};

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

	class WeaponShoot : public BaseSystem
	{
	public:
		void update(GameContext &context, float dt) override;
	private:
		std::mt19937 m_rng{std::random_device{}()};
		void fireRequestPreprocessing(GameContext &context, float dt);
		void assignIsFiringStatus(GameContext &context, float dt);
		void firingStatusPostprocessing(GameContext &context, float dt);
		void shootBullets(GameContext &context, float dt);
	};

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

	class HudWarning : public BaseSystem
	{
	public:
		void update(GameContext &context, float dt) override;
	private:
		float m_prevHp = 0.0f;
		float m_lowHpWarningDuration = 0.0f;
		float m_lowHpWarningCooldown = 0.0f;
		float m_lowHpBlinkTimer = 0.0f;

		float m_collisionAlertCooldown = 0.0f;
		float m_collisionBlinkTimer = 0.0f;

		float m_missileAlertCooldown = 0.0f;
		float m_missileBlinkTimer = 0.0f;

		bool updateLowHpWarning(GameContext &context, float dt, entt::entity targetEntity);
		bool updateCollisionWarning(GameContext &context, float dt, entt::entity targetEntity);
		bool updateMissileWarning(GameContext &context, float dt, entt::entity targetEntity);
	};

	class SoundSfx : public BaseSystem
	{
	public:
		void update(GameContext &context, float dt) override;
	private:
		float m_prevSpeed = 0.0f;
		entt::entity m_lastLockOnTarget = entt::null;

		void lockOnSfx(GameContext &context);
	};
} // namespace systems