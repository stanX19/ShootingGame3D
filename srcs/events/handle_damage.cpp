#include "events.hpp"
#include "components.hpp"

void event::Listener::handleCollisionDamage(const CollisionEvent &evt)
{
	auto applyDamage = [&](entt::entity victim, entt::entity killer)
	{
		HP *hpPtr = evt.context->registry.try_get<HP>(victim);
		Damage *dmgPtr = evt.context->registry.try_get<Damage>(killer);
		EnergyShield *shieldPtr = evt.context->registry.try_get<EnergyShield>(victim);

		if (!hpPtr || !dmgPtr)
			return ;
			
		float dmg = dmgPtr->value;

		// use shield to block if its energy weapon
		if (evt.context->registry.all_of<tag::bullet_type::Energy>(killer) && shieldPtr && shieldPtr->hp > 0) {
			if (shieldPtr->hp > dmg) {  // can block all damage
				shieldPtr->hp -= dmg;
				shieldPtr->activeTimer = 3.0f;  // TODO: read from constants instead
				return ;
			}
			dmg -= shieldPtr->hp;  // cannot block all
			shieldPtr->hp = 0;
		}
		if (hpPtr->value > 0 && dmg > hpPtr->value)
		{
			evt.context->dispatcher.enqueue<KillEvent>(KillEvent{
				evt.context,
				killer,
				victim
			});
		}
		hpPtr->value -= dmg;
	};

	applyDamage(evt.a, evt.b);
	applyDamage(evt.b, evt.a);
}