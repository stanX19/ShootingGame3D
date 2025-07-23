#include "events.hpp"
#include "components.hpp"
#include "entities.hpp"

namespace {
	void debrisFX(const event::CollisionEvent& evt) {
		auto& registry = evt.context->registry;

		auto trySpawn = [&](entt::entity victim, entt::entity damager, Vector3 victimPos, Vector3 victimVel, Vector3 damagerPos, Vector3 damagerVel) {
			auto [hpPtr, posPtr, bodyPtr] = registry.try_get<HP, Position, RenderBody>(victim);
			auto dmgPtr = registry.try_get<Damage>(damager);

			if (hpPtr && hpPtr->value > 0 && dmgPtr && dmgPtr->value >= 0 && bodyPtr && posPtr) {
				Vector3 normal = Vector3Normalize(damagerPos - victimPos);

				float scale = std::cbrt(bodyPtr->scale.x * bodyPtr->scale.y * bodyPtr->scale.z);
				int debrisCount = static_cast<int>(scale * dmgPtr->value / hpPtr->maxValue * 100);

				Vector3 collisionPos = posPtr->value + normal * scale;
				Vector3 explosionDir = Vector3Normalize(damagerVel) * -50 + victimVel;
				Color color = ColorLerp(bodyPtr->color, WHITE, 0.5f);

				spawnDebris(*evt.context, collisionPos, scale * 0.5f, color, debrisCount, 5.0f, explosionDir);
			}
		};

		trySpawn(evt.a, evt.b, evt.posA, evt.velA, evt.posB, evt.velB);  // A damaged by B
		trySpawn(evt.b, evt.a, evt.posB, evt.velB, evt.posA, evt.velA);  // B damaged by A
	}
}

void event::Listener::handleCollisionFX(const CollisionEvent& evt) {
	debrisFX(evt);
}
