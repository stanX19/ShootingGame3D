#include "events.hpp"
#include "components.hpp"

void event::Listener::handleCollisionDamage(const CollisionEvent &evt)
{
	auto applyDamage = [&](entt::entity victim, entt::entity killer)
	{
		HP *hpPtr = evt.context->registry.try_get<HP>(victim);
		Damage *dmgPtr = evt.context->registry.try_get<Damage>(killer);

		if (hpPtr && dmgPtr)
		{
			if (hpPtr->value > 0 && dmgPtr->value > hpPtr->value)
			{
				evt.context->dispatcher.enqueue<KillEvent>(KillEvent{
					evt.context,
					killer,
					victim
				});
			}
			hpPtr->value -= dmgPtr->value;
		}
	};

	applyDamage(evt.a, evt.b);
	applyDamage(evt.b, evt.a);
}