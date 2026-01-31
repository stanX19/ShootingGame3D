#ifndef LOOPING_FADE_SOUND_HPP
#define LOOPING_FADE_SOUND_HPP

#include "includes.hpp"
#include <string>

class LoopingFadeSound {
public:
	LoopingFadeSound(const std::string& path, float restartThreshold = 2.0f);
	~LoopingFadeSound();

	void init(float masterVolume = 1.0f);
	void shutdown();

	// Call every frame with whether sound should be playing
	void update(bool shouldPlay, float dt, float masterVolume = 1.0f);

	bool isLoaded() const { return loaded; }
	bool isPlaying() const { return playing; }

	void setFadeInDuration(float duration) { fadeInDuration = duration; }
	void setFadeOutDuration(float duration) { fadeOutDuration = duration; }
	void setMaxVolume(float volume) { maxVolume = volume; }

private:
	enum class FadeState {
		Idle,
		FadingIn,
		FadingOut
	};

	std::string soundPath;
	Sound sound;
	bool loaded = false;
	bool playing = false;

	FadeState fadeState = FadeState::Idle;
	float currentVolume = 0.0f;
	float maxVolume = 0.5f;
	float fadeInDuration = 0.3f;
	float fadeOutDuration = 1.0f;
	float restartThreshold;  // seconds after stop before restarting from beginning
	float timeSinceFadeOut = 0.0f;

	void startPlaying();
	void stopPlaying();
};

#endif  // LOOPING_FADE_SOUND_HPP
