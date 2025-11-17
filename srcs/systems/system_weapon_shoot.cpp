#include "systems.hpp"
#include "entities.hpp"
#include "utils.hpp"
#include "entt_utils.hpp"
#include "components/factions.hpp"
#include <random>

namespace
{
	std::mt19937 rng(std::random_device{}());
}

void ecs_systems::weaponShoot(GameContext &context, [[maybe_unused]] float dt)
{
    auto view = context.registry.view<Weapon, Position, AimDirection,
                                      tag::weapon::IsFiring, tag::weapon::CanFire>();

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

        context.registry.emplace_or_replace<JustFired>(entity, JustFired{1});
    }
}
