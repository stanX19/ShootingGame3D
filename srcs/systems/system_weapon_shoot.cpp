#include "systems.hpp"
#include "game_context.hpp"
#include "entities.hpp"
#include "entt_utils.hpp"
#include "components/physics.hpp"
#include "components/weapon.hpp"
#include "components/score.hpp"
#include "components/factions.hpp"
#include "components/sound.hpp"
#include "components/collision.hpp"
#include "utils/math_utils.hpp"
#include "utils/vector_rotation_utils.hpp"
#include "events.hpp"
#include <random>

void systems::WeaponShoot::fireRequestPreprocessing(GameContext &context, float dt) {
	// extend fire request
	for (auto [entity, fireRequest] : context.registry.view<ExtendFireRequest>().each()) {
		fireRequest.timeRemaining -= dt;
	}
	for (auto [entity, weapon, fireRequest] : context.registry.view<Weapon, ExtendFireRequest, tag::weapon::FireRequest, tag::weapon::CanFire>().each()) {
		fireRequest.timeRemaining = fireRequest.duration;
	}
	for (auto [entity, weapon, fireDuration] : context.registry.view<Weapon, ExtendFireRequest, tag::weapon::CanFire>().each()) {
		if (fireDuration.timeRemaining <= 0.0f)
			continue;
		context.registry.emplace_or_replace<tag::weapon::FireRequest>(entity);
	}

	// collect fire requests from normal firing
	for (auto [entity, weapon, chargedWeapon] : context.registry.view<Weapon, ChargedWeapon>(entt::exclude<tag::weapon::FireRequest>).each()) {
		chargedWeapon.currentCharge = 0.0f;
	}
	for (auto [entity, weapon, chargedWeapon] : context.registry.view<Weapon, ChargedWeapon, tag::weapon::FireRequest, tag::weapon::CanFire>().each()) {
		if (chargedWeapon.currentCharge < chargedWeapon.totalChargeNeeded) {
			context.registry.remove<tag::weapon::FireRequest>(entity);
			chargedWeapon.currentCharge += dt;
			Ammo *ammoPtr = context.registry.try_get<Ammo>(entity);
			if (ammoPtr)
				ammoPtr->value -= chargedWeapon.chargeAmmo * (dt / chargedWeapon.totalChargeNeeded);
		} else { // dont intercept if already fully charged
			chargedWeapon.currentCharge = 0.0f;
		}
	}
}

void systems::WeaponShoot::assignIsFiringStatus(GameContext &context, [[maybe_unused]] float dt) {
	// core fire request processing
	for (auto entity : context.registry.view<Weapon, tag::weapon::FireRequest, tag::weapon::CanFire>()) {
		context.registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
		context.registry.emplace_or_replace<JustFired>(entity, JustFired{1});
	}
	for (auto entity : context.registry.view<Weapon, tag::weapon::FireRequest>()) {
		context.registry.remove<tag::weapon::FireRequest>(entity);
	}
}

void systems::WeaponShoot::firingStatusPostprocessing(GameContext &context, float dt) {
	// continue IsFiring status for entities with remaining fire duration
	for (auto [entity, fireDuration] : context.registry.view<ExtendFireDuration>().each()) {
		fireDuration.timeRemaining -= dt;
	}
	for (auto [entity, weapon, fireDuration] : context.registry.view<Weapon, ExtendFireDuration, tag::weapon::IsFiring>().each()) {
		fireDuration.timeRemaining = fireDuration.duration;
	}
	for (auto [entity, weapon, fireDuration] : context.registry.view<Weapon, ExtendFireDuration>().each()) {
		if (fireDuration.timeRemaining <= 0.0f)
			continue;
		context.registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
	}
}

void systems::WeaponShoot::shootBullets(GameContext &context, [[maybe_unused]] float dt)
{
	auto view = context.registry.view<Weapon, Position, AimDirection, tag::weapon::IsFiring>();

	for (auto entity : view)
	{
		const auto &weapon = view.get<Weapon>(entity);
		const Vector3 pos = view.get<Position>(entity).value;
		const Vector3 baseDir = view.get<AimDirection>(entity).value;
		faction::FacVal faction = faction::FAC_NONE;
		if (const faction::Faction *factPtr = context.registry.try_get<faction::Faction>(entity))
			faction = faction | factPtr->value;
		const Velocity *velocityPtr = context.registry.try_get<Velocity>(entity);
		const Vector3 shooterVel = velocityPtr ? velocityPtr->value : Vector3Zeros;

		// Emit shoot sound if weapon has ShootSound component and belongs to player
		if (entt_utils::getRootScoreParent(context.registry, entity) == context.currentPlayer) {
			if (auto *shootSound = context.registry.try_get<sound::ShootSound>(entity)) {
				if (shootSound->id != sound::NONE) {
					context.dispatcher.enqueue<event::SoundEvent>(event::SoundEvent{
						&context,
						shootSound->id,
						pos,
						shootSound->volume
					});
				}
			}
		}

		for (int i = 0; i < weapon.bulletData.bulletCount; i++)
		{
			std::uniform_real_distribution<float> weaponSpreadDistribution(0, weapon.bulletData.spreadSin);
			const Vector3 offset = Vector3Normalize(
				Vector3CrossProduct(randomUnitVector3(), Vector3Normalize(baseDir))
			) * weaponSpreadDistribution(m_rng);

			const Vector3 dir = Vector3Normalize(baseDir + offset);

			float rad = 0.0f;
			
			if (context.registry.any_of<CollisionBody>(entity))
				rad = context.registry.get<CollisionBody>(entity).radius + context.templateReg.get<CollisionBody>(weapon.bulletTemplate).radius + 1.0f;

			const entt::entity bullet = entt_utils::cloneEntity(context.templateReg, weapon.bulletTemplate, context.registry);
			context.registry.emplace_or_replace<Position>(bullet, Position{pos + dir * (rad + 0.1f)});
			context.registry.emplace_or_replace<Velocity>(bullet, Velocity{dir * weapon.bulletData.speed + shooterVel});
			context.registry.emplace_or_replace<ScoreParent>(bullet, ScoreParent{entity});
			context.registry.emplace_or_replace<faction::Faction>(bullet, faction);
		}
		context.registry.remove<tag::weapon::IsFiring>(entity);
	}
}

void systems::WeaponShoot::update(GameContext &context, float dt)
{
	fireRequestPreprocessing(context, dt);
	assignIsFiringStatus(context, dt);
	firingStatusPostprocessing(context, dt);
	shootBullets(context, dt);
}
