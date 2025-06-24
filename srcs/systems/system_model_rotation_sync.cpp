#include "shoot_3d.hpp"

void ecs_systems::model_rotation_sync(GameContext &context) {
    auto view = context.registry.view<Rotation, RenderBody>();

    for (auto entity : view) {
        Rotation& rotation = view.get<Rotation>(entity);
		RenderBody& body = view.get<RenderBody>(entity);
		
		body.rotation = rotation.value;
    }
}
