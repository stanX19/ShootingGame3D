#include "systems.hpp"
#include "entities.hpp"
#include "utils.hpp"
#include "entt_utils.hpp"
#include <random>

namespace
{
	std::mt19937 rng(std::random_device{}());
}

void ecs_systems::weaponShoot(GameContext &context)
{
    auto view = context.registry.view<BulletWeapon, Position, AimDirection,
                                      tag::weapon::IsFiring, tag::weapon::CanFire>();

    for (auto entity : view)
    {
        auto &weapon = view.get<BulletWeapon>(entity);
        Vector3 pos = view.get<Position>(entity).value;
        Vector3 baseDir = view.get<AimDirection>(entity).value;

        for (int i = 0; i < weapon.bulletData.bulletCount; i++)
        {
            std::uniform_real_distribution<float> dist(0, weapon.bulletData.spreadSin);
            Vector3 offset = Vector3Normalize(
                Vector3CrossProduct(randomUnitVector3(), Vector3Normalize(baseDir))
            ) * dist(rng);

            Vector3 dir = Vector3Normalize(baseDir + offset);

            float rad = 0.0f;
			
			if (context.registry.any_of<CollisionBody>(entity))
                rad = context.registry.get<CollisionBody>(entity).radius + context.templateReg.get<CollisionBody>(weapon.bulletTemplate).radius + 1.0f;

            entt::entity bullet = entt_utils::cloneEntity(context.templateReg, weapon.bulletTemplate, context.registry);
            context.registry.emplace_or_replace<Position>(bullet, Position{pos + dir * rad});
            context.registry.emplace_or_replace<Velocity>(bullet, Velocity{dir * weapon.bulletData.speed});
            context.registry.emplace_or_replace<ScoreParent>(bullet, ScoreParent{entity});
        }

        context.registry.emplace_or_replace<JustFired>(entity, JustFired{1});
    }
}
