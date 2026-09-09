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

	bool isLoaded() const { return m_loaded; }
	bool isPlaying() const { return m_playing; }

	void setFadeInDuration(float duration) { m_fadeInDuration = duration; }
	void setFadeOutDuration(float duration) { m_fadeOutDuration = duration; }
	void setMaxVolume(float volume) { m_maxVolume = volume; }

private:
	enum class FadeState {
		Idle,
		FadingIn,
		FadingOut
	};

	std::string m_soundPath;
	Sound m_sound;
	bool m_loaded = false;
	bool m_playing = false;

	FadeState m_fadeState = FadeState::Idle;
	float m_currentVolume = 0.0f;
	float m_maxVolume = 0.5f;
	float m_fadeInDuration = 0.3f;
	float m_fadeOutDuration = 1.0f;
	float m_restartThreshold;  // seconds after stop before restarting from beginning
	float m_timeSinceFadeOut = 0.0f;

	void startPlaying();
	void stopPlaying();
};

#endif  // LOOPING_FADE_SOUND_HPP
