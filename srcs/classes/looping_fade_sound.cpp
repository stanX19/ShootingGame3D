#include "looping_fade_sound.hpp"
#include <algorithm>

LoopingFadeSound::LoopingFadeSound(const std::string& path, float restartThreshold)
	: m_soundPath(path), m_restartThreshold(restartThreshold) {}

LoopingFadeSound::~LoopingFadeSound() {
	shutdown();
}

void LoopingFadeSound::init(float masterVolume) {
	if (m_loaded || m_soundPath.empty())
		return;

	m_sound = LoadSound(m_soundPath.c_str());
	m_loaded = IsSoundValid(m_sound);
	if (m_loaded) {
		SetSoundVolume(m_sound, 0.0f);
	}
	(void)masterVolume;
}

void LoopingFadeSound::shutdown() {
	if (!m_loaded)
		return;

	StopSound(m_sound);
	UnloadSound(m_sound);
	m_loaded = false;
	m_playing = false;
	m_fadeState = FadeState::Idle;
	m_currentVolume = 0.0f;
}

void LoopingFadeSound::startPlaying() {
	if (!m_loaded || m_playing)
		return;

	PlaySound(m_sound);
	m_playing = true;
}

void LoopingFadeSound::stopPlaying() {
	if (!m_loaded || !m_playing)
		return;

	StopSound(m_sound);
	m_playing = false;
}

void LoopingFadeSound::update(bool shouldPlay, float dt, float masterVolume) {
	if (!m_loaded)
		return;

	// Handle state transitions
	if (shouldPlay && m_fadeState != FadeState::FadingIn) {
		// Check if we should restart from beginning
		if (m_fadeState == FadeState::FadingOut && m_timeSinceFadeOut >= m_restartThreshold) {
			stopPlaying();
			m_currentVolume = 0.0f;
		}
		m_fadeState = FadeState::FadingIn;
		m_timeSinceFadeOut = 0.0f;
	} else if (!shouldPlay && m_fadeState == FadeState::FadingIn) {
		m_fadeState = FadeState::FadingOut;
		m_timeSinceFadeOut = 0.0f;
	}

	// Track time since fade out started
	if (m_fadeState == FadeState::FadingOut) {
		m_timeSinceFadeOut += dt;
	}

	// Update volume based on state
	switch (m_fadeState) {
		case FadeState::FadingIn: {
			if (!m_playing)
				startPlaying();

			float fadeSpeed = m_maxVolume / m_fadeInDuration;
			m_currentVolume = std::min(m_currentVolume + fadeSpeed * dt, m_maxVolume);

			if (m_currentVolume >= m_maxVolume) {
				m_currentVolume = m_maxVolume;
			}
			break;
		}
		case FadeState::FadingOut: {
			float fadeSpeed = m_maxVolume / m_fadeOutDuration;
			m_currentVolume = std::max(m_currentVolume - fadeSpeed * dt, 0.0f);

			if (m_currentVolume <= 0.0f) {
				m_currentVolume = 0.0f;
				stopPlaying();
				m_fadeState = FadeState::Idle;
			}
			break;
		}
		case FadeState::Idle:
			break;
	}

	// Loop sound if it finished but should still be playing
	if (m_playing && !IsSoundPlaying(m_sound) && m_fadeState == FadeState::FadingIn) {
		PlaySound(m_sound);
	}

	// Apply volume
	if (m_playing) {
		SetSoundVolume(m_sound, m_currentVolume * masterVolume);
	}
}
