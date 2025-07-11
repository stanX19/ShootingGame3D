#include "events.hpp"
#include "components.hpp"
#include "entities.hpp"

void event::Listener::handleCollisionFX(const CollisionEvent& evt) {
	auto& registry = evt.context->registry;

	auto trySpawn = [&](entt::entity victim, entt::entity damager) {
		auto [hpPtr, prevPosPtr, posPtr, bodyPtr] = registry.try_get<HP, PrevPosition, Position, RenderBody>(victim);
		auto [dmgPtr, damagerPrevPosPtr, damagerPosPtr] = registry.try_get<Damage, PrevPosition, Position>(damager);

		if (hpPtr && dmgPtr && dmgPtr->value >= 0 && prevPosPtr && bodyPtr && damagerPrevPosPtr && posPtr) {
			Vector3 victimPos = Vector3Lerp(prevPosPtr->value, posPtr->value, evt.collisionDt / evt.dt);
			Vector3 damagerPos = Vector3Lerp(damagerPrevPosPtr->value, damagerPosPtr->value, evt.collisionDt / evt.dt);
			Vector3 normal = Vector3Normalize(victimPos - damagerPos);
			float scale = std::cbrt(bodyPtr->scale.x * bodyPtr->scale.y * bodyPtr->scale.z);
			Vector3 collisionPos = posPtr->value + normal * scale;
			Vector3 explosionDir = normal * 50 + (posPtr->value - prevPosPtr->value) / evt.dt;
			int debrisCount = static_cast<int>(scale * 5.0f);
			Color color = ColorLerp(bodyPtr->color, WHITE, 0.5f);
			spawnDebris(*evt.context, collisionPos, scale * 0.5f, color, debrisCount, 5.0f, explosionDir);
		}
	};

	trySpawn(evt.a, evt.b);  // A damaged by B
	trySpawn(evt.b, evt.a);  // B damaged by A
}
