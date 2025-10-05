#include "systems.hpp"

namespace {
	void transformRadius(GameContext &context, float dt) {
		for (auto [entity, body, expand] : context.registry.view<RenderBody, RadiusExpand>().each()) {
			float maxScale = std::max(body.scale.x, std::max(body.scale.y, body.scale.z));
			body.scale *= (maxScale + expand.speed * dt) / maxScale;
		}
		for (auto [entity, body, expand] : context.registry.view<CollisionBody, RadiusExpand>().each()) {
			body.radius += expand.speed * dt;
		}
	}
}

void ecs_systems::entityTransformation(GameContext &context, float dt) {
	transformRadius(context, dt);
}