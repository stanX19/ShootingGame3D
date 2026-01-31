#include "looping_fade_sound.hpp"
#include <algorithm>

LoopingFadeSound::LoopingFadeSound(const std::string& path, float restartThreshold)
	: soundPath(path), restartThreshold(restartThreshold) {}

LoopingFadeSound::~LoopingFadeSound() {
	shutdown();
}

void LoopingFadeSound::init(float masterVolume) {
	if (loaded || soundPath.empty())
		return;

	sound = LoadSound(soundPath.c_str());
	loaded = IsSoundValid(sound);
	if (loaded) {
		SetSoundVolume(sound, 0.0f);
	}
	(void)masterVolume;
}

void LoopingFadeSound::shutdown() {
	if (!loaded)
		return;

	StopSound(sound);
	UnloadSound(sound);
	loaded = false;
	playing = false;
	fadeState = FadeState::Idle;
	currentVolume = 0.0f;
}

void LoopingFadeSound::startPlaying() {
	if (!loaded || playing)
		return;

	PlaySound(sound);
	playing = true;
}

void LoopingFadeSound::stopPlaying() {
	if (!loaded || !playing)
		return;

	StopSound(sound);
	playing = false;
}

void LoopingFadeSound::update(bool shouldPlay, float dt, float masterVolume) {
	if (!loaded)
		return;

	// Handle state transitions
	if (shouldPlay && fadeState != FadeState::FadingIn) {
		// Check if we should restart from beginning
		if (fadeState == FadeState::FadingOut && timeSinceFadeOut >= restartThreshold) {
			stopPlaying();
			currentVolume = 0.0f;
		}
		fadeState = FadeState::FadingIn;
		timeSinceFadeOut = 0.0f;
	} else if (!shouldPlay && fadeState == FadeState::FadingIn) {
		fadeState = FadeState::FadingOut;
		timeSinceFadeOut = 0.0f;
	}

	// Track time since fade out started
	if (fadeState == FadeState::FadingOut) {
		timeSinceFadeOut += dt;
	}

	// Update volume based on state
	switch (fadeState) {
		case FadeState::FadingIn: {
			if (!playing)
				startPlaying();

			float fadeSpeed = maxVolume / fadeInDuration;
			currentVolume = std::min(currentVolume + fadeSpeed * dt, maxVolume);

			if (currentVolume >= maxVolume) {
				currentVolume = maxVolume;
			}
			break;
		}
		case FadeState::FadingOut: {
			float fadeSpeed = maxVolume / fadeOutDuration;
			currentVolume = std::max(currentVolume - fadeSpeed * dt, 0.0f);

			if (currentVolume <= 0.0f) {
				currentVolume = 0.0f;
				stopPlaying();
				fadeState = FadeState::Idle;
			}
			break;
		}
		case FadeState::Idle:
			break;
	}

	// Loop sound if it finished but should still be playing
	if (playing && !IsSoundPlaying(sound) && fadeState == FadeState::FadingIn) {
		PlaySound(sound);
	}

	// Apply volume
	if (playing) {
		SetSoundVolume(sound, currentVolume * masterVolume);
	}
}
