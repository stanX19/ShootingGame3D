#include "shoot_3d.hpp"


void ecs_systems::hpCleanup(GameContext &context) {
    auto view = context.registry.view<HP>();
    std::vector<entt::entity> toDestroy;

    for (auto entity : view) {
        HP& hp = view.get<HP>(entity);
        if (hp.value <= 0.001f) {
            toDestroy.push_back(entity);
        }
    }

    for (auto entity : toDestroy) {
        if (!context.registry.valid(entity)) continue;

        if (context.registry.all_of<Position, RenderBody>(entity)) {
            const Position& pos = context.registry.get<Position>(entity);
            const RenderBody& renderBody = context.registry.get<RenderBody>(entity);
            spawnDebris(context, pos.value, renderBody.scale, renderBody.color, (int)(renderBody.scale * 25), 5.0);
        }

        context.registry.destroy(entity);
    }
}