#include "events.hpp"
#include "components.hpp"

void event::Listener::handleCollisionDamage(const CollisionEvent& evt) {
	auto [hpA, dmgA] = evt.context->registry.try_get<HP, Damage>(evt.a);
	auto [hpB, dmgB] = evt.context->registry.try_get<HP, Damage>(evt.b);

	if (hpA && dmgB)
		hpA->value -= dmgB->value * evt.dt;
	if (hpB && dmgA)
		hpB->value -= dmgA->value * evt.dt;
}
