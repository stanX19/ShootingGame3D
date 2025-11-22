#include "events.hpp"
#include "components.hpp"
#include "entities.hpp"

namespace {
	void trySpawnDebris(GameContext *context, const event::CollisionParty& victim, 
	                   const event::CollisionParty& damager) {
		entt::registry &registry = context->registry;
		
		if (!registry.any_of<tag::effect::DropDebris>(victim.id))
			return;

		auto [hpPtr, posPtr, bodyPtr] = registry.try_get<HP, Position, RenderBody>(victim.id);
		auto dmgPtr = registry.try_get<Damage>(damager.id);

		if (!hpPtr || hpPtr->value <= 0 || !dmgPtr || dmgPtr->value < 0 || !bodyPtr || !posPtr)
			return;
			
		Vector3 normal = Vector3Normalize(damager.pos - victim.pos);

		float scale = std::cbrt(bodyPtr->scale.x * bodyPtr->scale.y * bodyPtr->scale.z);
		int debrisCount = 2 + static_cast<int>(30 * std::min(1.0f, dmgPtr->value / hpPtr->maxValue));

		Vector3 collisionPos = posPtr->value + normal * scale;
		Vector3 explosionDir = Vector3Normalize(damager.vel) * -50 + victim.vel;
		Color color = ColorLerp(bodyPtr->color, WHITE, 0.5f);

		spawnDebris(*context, collisionPos, scale, color, debrisCount, 5.0f, explosionDir);
	}

	void applyDamageToEntity(const event::CollisionEvent &evt, const event::CollisionParty& victim, 
	                        const event::CollisionParty& killer) {
		entt::registry &registry = evt.context->registry;
		
		HP *hpPtr = registry.try_get<HP>(victim.id);
		Damage *dmgPtr = registry.try_get<Damage>(killer.id);
		EnergyShield *shieldPtr = registry.try_get<EnergyShield>(victim.id);

		if (!hpPtr || !dmgPtr || hpPtr->value <= 0)
			return;
			
		float dmg = dmgPtr->value;

		// Use shield to block if it's an energy weapon
		if (registry.all_of<tag::bullet_type::Energy>(killer.id) && shieldPtr && shieldPtr->hp > 0) {
			if (shieldPtr->hp > dmg) {  // Can block all damage
				shieldPtr->hp -= dmg;
				shieldPtr->activeTimer = shieldPtr->activeDuration;
				return;
			}
			dmg -= shieldPtr->hp;  // Cannot block all damage
			shieldPtr->hp = 0;
		}

		hpPtr->value -= dmg;

		if (hpPtr->value < 0) {
			evt.context->dispatcher.enqueue<event::KillEvent>(event::KillEvent{
				evt.context,
				killer,
				victim,
				evt.dt
			});
		}
		
		// Try to spawn debris after applying damage
		trySpawnDebris(evt.context, victim, killer);
	}
}

void event::Listener::handleCollisionEvent(const CollisionEvent &evt) {
	// std::cout << evt.a.pos.x << " " << evt.b.pos.x << std::endl;
	applyDamageToEntity(evt, evt.b, evt.a);
	applyDamageToEntity(evt, evt.a, evt.b);
}