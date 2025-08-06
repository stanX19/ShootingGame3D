#include "systems.hpp"

namespace {
	void transformRadius(GameContext &context, float dt) {
		for (auto [entity, body, expand] : context.registry.view<RenderBody, RadiusExpand>().each()) {
			body.scale += Vector3Ones * expand.speed * dt;
		}
		for (auto [entity, body, expand] : context.registry.view<CollisionBody, RadiusExpand>().each()) {
			body.radius += expand.speed * dt;
		}
	}
}

void ecs_systems::entityTransformation(GameContext &context, float dt) {
	transformRadius(context, dt);
}