#include "events.hpp"
#include "components.hpp"
#include "entities.hpp"


void event::Listener::handleCollisionFX(const CollisionEvent& evt) {
	auto& registry = evt.context->registry;

	auto trySpawn = [&](entt::entity victim, entt::entity damager) {
		auto [hpPtr, prevPosPtr, velocityPtr, renderPtr] = registry.try_get<HP, PrevPosition, Velocity, RenderBody>(victim);
		auto dmgPtr = registry.try_get<Damage>(damager);
		auto [damagerPrevPosPtr, damagerVelPtr] = registry.try_get<PrevPosition, Velocity>(damager);

		if (hpPtr && dmgPtr && dmgPtr->value >= 0 && prevPosPtr && velocityPtr && renderPtr && damagerPrevPosPtr && damagerVelPtr) {
			Vector3 victimPos = prevPosPtr->value + velocityPtr->value * evt.collisionDt;
			Vector3 damagerPos = damagerPrevPosPtr->value + damagerVelPtr->value * evt.collisionDt;
			Vector3 collisionPos = (victimPos + damagerPos) / 2.0f;
			Vector3 normal = Vector3Normalize(damagerPos - victimPos);
			Vector3 explosionDir = normal * 3 + velocityPtr->value;
			float scale = std::cbrt(renderPtr->scale.x * renderPtr->scale.y * renderPtr->scale.z) * 0.5f;
			int debrisCount = static_cast<int>(scale * 5.0f);
			Color color = ORANGE;//ColorLerp(renderPtr->color, ORANGE, 0.25f);
			spawnDebris(*evt.context, collisionPos, scale, color, debrisCount, 5.0f, explosionDir);
		}
	};

	trySpawn(evt.a, evt.b);  // A damaged by B
	trySpawn(evt.b, evt.a);  // B damaged by A
}
