#include "events.hpp"

void event::utils::hookAllListeners(GameContext& context)
{
	Listener listener;
	context.dispatcher.sink<CollisionEvent>().connect<&Listener::handleCollisionDamage>(listener);
}