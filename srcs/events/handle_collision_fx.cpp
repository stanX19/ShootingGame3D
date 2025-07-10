#include "events.hpp"
#include "components.hpp"
#include "entities.hpp"

void event::Listener::handleCollisionFX(const CollisionEvent& evt) {
	auto& registry = evt.context->registry;

	auto trySpawn = [&](entt::entity victim, entt::entity damager) {
		auto [hpPtr, prevPosPtr, posPtr, velocityPtr, renderPtr] = registry.try_get<HP, PrevPosition, Position, Velocity, RenderBody>(victim);
		auto [dmgPtr, damagerPrevPosPtr, damagerVelPtr] = registry.try_get<Damage, PrevPosition, Velocity>(damager);

		if (hpPtr && dmgPtr && dmgPtr->value >= 0 && prevPosPtr && velocityPtr && renderPtr && damagerPrevPosPtr && damagerVelPtr && posPtr) {
			Vector3 victimPos = prevPosPtr->value + velocityPtr->value * evt.collisionDt;
			Vector3 damagerPos = damagerPrevPosPtr->value + damagerVelPtr->value * evt.collisionDt;
			Vector3 normal = Vector3Normalize(damagerPos - victimPos);
			float scale = std::cbrt(renderPtr->scale.x * renderPtr->scale.y * renderPtr->scale.z);
			Vector3 collisionPos = posPtr->value + normal * scale;
			Vector3 explosionDir = normal * 10 + velocityPtr->value;
			int debrisCount = static_cast<int>(scale * 5.0f);
			Color color = ColorLerp(renderPtr->color, WHITE, 0.5f);
			spawnDebris(*evt.context, collisionPos, scale * 0.5f, color, debrisCount, 5.0f, explosionDir);
		}
	};

	trySpawn(evt.a, evt.b);  // A damaged by B
	trySpawn(evt.b, evt.a);  // B damaged by A
}
