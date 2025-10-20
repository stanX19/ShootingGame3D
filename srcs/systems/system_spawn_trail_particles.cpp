#include "systems.hpp"
#include "entities.hpp"

void ecs_systems::spawnTrailParticles(GameContext &context, [[maybe_unused]] float dt) {
    for (auto [entity, trail, pos, vel] : context.registry.view<SpawnsTrailParticle, Position, Velocity>().each()) {
        if (Vector3Length(vel.value) > 10.0f) {
            Vector3 particlePos = pos.value - Vector3Normalize(vel.value) * trail.radius * 2.0f;

            entt::entity particle = context.registry.create();
            context.registry.emplace<Position>(particle, Position{particlePos});
            context.registry.emplace<Velocity>(particle, Velocity{vel.value * 0.5f});
            context.registry.emplace<RenderBody>(particle, context.modelManager.createSphere(), ColorAlpha(SKYBLUE, 0.5f), trail.radius);
            context.registry.emplace<RadiusExpand>(particle, -trail.radius / trail.lifespan);
            context.registry.emplace<Lifespan>(particle, trail.lifespan);
        }
    }
}