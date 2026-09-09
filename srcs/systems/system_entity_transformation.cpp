#include "systems.hpp"
#include "game_context.hpp"
#include "components/render.hpp"
#include "components/collision.hpp"
#include <algorithm>

namespace {
	void transformRadius(GameContext &context, float dt) {
		for (auto [entity, body, expand] : context.registry.view<RenderBody, const RadiusExpand>().each()) {
			const float maxScale = std::max(body.scale.x, std::max(body.scale.y, body.scale.z));
			body.scale *= (maxScale + expand.speed * dt) / maxScale;
		}
		for (auto [entity, body, expand] : context.registry.view<CollisionBody, const RadiusExpand>().each()) {
			body.radius += expand.speed * dt;
		}
	}
}

void systems::EntityTransformation::update(GameContext &context, float dt) {
	transformRadius(context, dt);
}