#include "systems.hpp"
#include "utils.hpp"

void ecs_systems::sync_model_rotation(GameContext &context) {
    auto view1 = context.registry.view<Rotation, RenderBody, tag::RotationSyncModel>();

    for (auto entity : view1) {
        Rotation& rotation = view1.get<Rotation>(entity);
		RenderBody& body = view1.get<RenderBody>(entity);
		
		body.rotation = rotation.value;
    }

    auto view2 = context.registry.view<AimDirection, RenderBody, tag::AimDirectionSyncModel>();

    for (auto entity : view2) {
        AimDirection& aimDirection = view2.get<AimDirection>(entity);
		RenderBody& body = view2.get<RenderBody>(entity);
		
		body.rotation = vector3ToRotation(aimDirection.value);
    }
}
