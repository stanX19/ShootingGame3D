#include "systems.hpp"
#include "entities.hpp"
#include "utils.hpp"
#include "entt_utils.hpp"
#include "components/factions.hpp"
#include <random>

namespace
{
	std::mt19937 rng(std::random_device{}());

	void assignIsFiringStatus(GameContext &context, [[maybe_unused]] float dt) {
		for (auto entity : context.registry.view<Weapon, tag::weapon::FireRequest, tag::weapon::CanFire>()) {
			context.registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
			context.registry.emplace_or_replace<JustFired>(entity, JustFired{1});
		}

		for (auto entity : context.registry.view<Weapon, tag::weapon::FireRequest>()) {
			context.registry.remove<tag::weapon::FireRequest>(entity);
		}
	}

	void updateFireDuration(GameContext &context, [[maybe_unused]] float dt) {
		for (auto [entity, fireDuration] : context.registry.view<FireDurationTakeAmmo>().each()) {
			fireDuration.timeRemaining -= dt;
		}
		for (auto [entity, fireDuration] : context.registry.view<FireDuration>().each()) {
			fireDuration.timeRemaining -= dt;
		}

		for (auto [entity, weapon, fireDuration] : context.registry.view<Weapon, FireDuration, tag::weapon::IsFiring>().each()) {
			fireDuration.timeRemaining = fireDuration.duration;
		}
		for (auto [entity, weapon, fireDuration] : context.registry.view<Weapon, FireDurationTakeAmmo, tag::weapon::IsFiring>().each()) {
			fireDuration.timeRemaining = fireDuration.duration;
		}

		// continue IsFiring status for entities with remaining fire duration
		for (auto [entity, weapon, fireDuration] : context.registry.view<Weapon, FireDuration>().each()) {
			if (fireDuration.timeRemaining <= 0.0f)
				continue;
			context.registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
		}
		for (auto [entity, weapon, fireDuration] : context.registry.view<Weapon, FireDurationTakeAmmo, tag::weapon::CanFire>().each()) {
			if (fireDuration.timeRemaining <= 0.0f)
				continue;
			context.registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
			context.registry.emplace_or_replace<JustFired>(entity, JustFired{1});
		}
	}

	void shootBullets(GameContext &context, [[maybe_unused]] float dt)
	{
		auto view = context.registry.view<Weapon, Position, AimDirection, tag::weapon::IsFiring>();

		for (auto entity : view)
		{
			auto &weapon = view.get<Weapon>(entity);
			Vector3 pos = view.get<Position>(entity).value;
			Vector3 baseDir = view.get<AimDirection>(entity).value;
			faction::FacVal faction = faction::FAC_NONE;
			if (faction::Faction *factPtr = context.registry.try_get<faction::Faction>(entity))
				faction = faction | factPtr->value;
			Velocity *velocityPtr = context.registry.try_get<Velocity>(entity);
			Vector3 shooterVel = velocityPtr ? velocityPtr->value : Vector3Zeros;

			for (int i = 0; i < weapon.bulletData.bulletCount; i++)
			{
				std::uniform_real_distribution<float> weaponSpreadDistribution(0, weapon.bulletData.spreadSin);
				Vector3 offset = Vector3Normalize(
					Vector3CrossProduct(randomUnitVector3(), Vector3Normalize(baseDir))
				) * weaponSpreadDistribution(rng);

				Vector3 dir = Vector3Normalize(baseDir + offset);

				float rad = 0.0f;
				
				if (context.registry.any_of<CollisionBody>(entity))
					rad = context.registry.get<CollisionBody>(entity).radius + context.templateReg.get<CollisionBody>(weapon.bulletTemplate).radius + 1.0f;

				entt::entity bullet = entt_utils::cloneEntity(context.templateReg, weapon.bulletTemplate, context.registry);
				context.registry.emplace_or_replace<Position>(bullet, Position{pos + dir * (rad + 0.1f)});
				context.registry.emplace_or_replace<Velocity>(bullet, Velocity{dir * weapon.bulletData.speed + shooterVel});
				context.registry.emplace_or_replace<ScoreParent>(bullet, ScoreParent{entity});
				context.registry.emplace_or_replace<faction::Faction>(bullet, faction);
			}
			context.registry.remove<tag::weapon::IsFiring>(entity);
		}
	}
}

void ecs_systems::weaponShoot(GameContext &context, [[maybe_unused]] float dt)
{
	assignIsFiringStatus(context, dt);
	updateFireDuration(context, dt);
    shootBullets(context, dt);
}
