#include "events.hpp"

void event::utils::hookAllListeners(GameContext& context)
{
	context.dispatcher.sink<CollisionEvent>().disconnect<&Listener::handleCollisionEvent>();
	context.dispatcher.sink<CollisionEvent>().connect<&Listener::handleCollisionEvent>();
	context.dispatcher.sink<KillEvent>().disconnect<&Listener::handleKillEvent>();
	context.dispatcher.sink<KillEvent>().connect<&Listener::handleKillEvent>();
	context.dispatcher.sink<SoundEvent>().disconnect<&Listener::handleSoundEvent>();
	context.dispatcher.sink<SoundEvent>().connect<&Listener::handleSoundEvent>();
}