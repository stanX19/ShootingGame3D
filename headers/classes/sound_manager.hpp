#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include "includes.hpp"
#include <map>
#include <vector>
#include <string>
#include <random>

namespace sound {
	// Sound ID type - simple int for cache efficiency in ECS
	using Id = int;
	constexpr Id NONE = 0;

	// Virtual IDs for randomized sounds (negative to avoid collision with loaded sounds)
	constexpr Id RANDOM_BULLET_SHOOT = -1;
	constexpr Id RANDOM_LAZER_SHOOT  = -2;
	constexpr Id RANDOM_BULLET_HIT   = -3;
	constexpr Id RANDOM_LAZER_HIT    = -4;
	constexpr Id RANDOM_EXPLOSION    = -5;
} // namespace sound

struct PlaySoundRequest {
	sound::Id id;
	Vector3 position;
	float volume;
};

class GameConfig;  // Forward declaration

class SoundManager {
public:
	SoundManager();
	~SoundManager();

	void init(const GameConfig& config);
	void shutdown();

	// Random getters - return specific ID (randomization happens at attach time)
	sound::Id getRandomBulletShoot();
	sound::Id getRandomLazerShoot();
	sound::Id getRandomBulletHit();
	sound::Id getRandomLazerHit();
	sound::Id getRandomExplosion();
	sound::Id getBackgroundMusic() const;

	// Queue a sound to be played (with throttling)
	void queueSound(sound::Id id, Vector3 position = {0,0,0}, float volume = 1.0f);

	// Process queued sounds (call once per frame after dispatcher.update())
	void update(const Camera3D& camera);

	// Direct play (bypasses throttling - use sparingly)
	void playImmediate(sound::Id id, float volume = 1.0f);

	// Background music
	void playMusic();
	void updateMusic();
	void stopMusic();

	void setMasterVolume(float volume);
	float getMasterVolume() const;

	void setEnabled(bool enabled);
	bool isEnabled() const;

private:
	int maxSoundsPerIdPerFrame = 3;
	int soundAliasesCount = 4;  // simultaneous plays per sound

	struct SoundData {
		Sound baseSound;
		std::vector<Sound> aliases;  // for simultaneous playback
		int currentAlias = 0;
		bool loaded = false;
	};

	std::map<sound::Id, SoundData> sounds;
	std::vector<PlaySoundRequest> pendingRequests;
	std::map<sound::Id, int> playsThisFrame;

	// Category pools - populated from config
	std::vector<sound::Id> bulletShootIds;
	std::vector<sound::Id> lazerShootIds;
	std::vector<sound::Id> bulletHitIds;
	std::vector<sound::Id> lazerHitIds;
	std::vector<sound::Id> explosionIds;
	sound::Id backgroundMusicId = sound::NONE;

	Music backgroundMusic;
	bool musicLoaded = false;

	float masterVolume = 0.5f;
	bool enabled = true;
	bool initialized = false;
	
	int nextSoundId = 1;  // Start at 1, 0 is NONE, negatives are virtual
	std::mt19937 rng{std::random_device{}()};

	sound::Id loadSound(const std::string& path);
	void unloadSound(sound::Id id);
	Sound& getNextAlias(sound::Id id);
	void loadFromConfig(const GameConfig& config);
	sound::Id getRandomFromPool(const std::vector<sound::Id>& pool);
	sound::Id resolveVirtualId(sound::Id id);  // Convert virtual IDs to actual random sounds
};

#endif  // SOUND_MANAGER_HPP
