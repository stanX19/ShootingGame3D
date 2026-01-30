#include "events.hpp"

void event::utils::hookAllListeners(GameContext& context)
{
	Listener listener;
	context.dispatcher.sink<CollisionEvent>().connect<&Listener::handleCollisionEvent>(listener);
	context.dispatcher.sink<KillEvent>().connect<&Listener::handleKillEvent>(listener);
	context.dispatcher.sink<SoundEvent>().connect<&Listener::handleSoundEvent>(listener);
}