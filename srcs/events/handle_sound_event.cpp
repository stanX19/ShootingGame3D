#include "events.hpp"

void event::Listener::handleSoundEvent(const SoundEvent& evt) {
	evt.context->soundManager.queueSound(evt.soundId, evt.position, evt.volume);
}
