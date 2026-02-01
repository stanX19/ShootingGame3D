#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include "includes.hpp"
#include "looping_fade_sound.hpp"
#include <map>
#include <vector>
#include <string>
#include <random>
#include <memory>

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
	constexpr Id RANDOM_MISSILE_SHOOT = -6;
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
	sound::Id getRandomMissileShoot();
	sound::Id getBackgroundMusic() const;
	sound::Id getAlertSound() const;
	sound::Id getThrustSound() const;


	// Process queued sounds (call once per frame after dispatcher.update())
	void update(const Camera3D& camera);

	// Queue a sound to be played (with throttling)
	void queueSound(sound::Id id, Vector3 position = {0,0,0}, float volume = 1.0f);
	void queueSound(const GameConfig& config, const std::string& configPath,
					Vector3 position = {0,0,0}, float volume = 1.0f);

	// Direct play (bypasses throttling - use sparingly)
	void playImmediate(sound::Id id, float volume = 1.0f);
	void playImmediate(const GameConfig& config, const std::string& configPath, float volume = 1.0f);

	// Load a sound file and return its ID
	sound::Id loadSound(const std::string& path);

	// Load a sound from config path (e.g., "sounds.warning")
	sound::Id loadSound(const GameConfig& config, const std::string& configPath, sound::Id defaultId = sound::NONE);

	// Background music
	void playMusic();
	void updateMusic();
	void stopMusic();

	// Thrust sound (looping with fade)
	void updateThrustSound(bool isAccelerating, float dt);

	void setMasterVolume(float volume);
	float getMasterVolume() const;

	void setEnabled(bool enabled);
	bool isEnabled() const;

private:
	int maxSoundsPerIdPerFrame = 3;
	int soundAliasesCount = 4;  // simultaneous plays per sound
	float explosionMaxDistance = -1.0f;  // -1 means no limit

	struct SoundData {
		Sound baseSound;
		std::vector<Sound> aliases;  // for simultaneous playback
		int currentAlias = 0;
		bool loaded = false;
	};

	std::map<sound::Id, SoundData> sounds;
	std::map<std::string, sound::Id> pathCache;  // path -> id cache
	std::vector<PlaySoundRequest> pendingRequests;
	std::map<sound::Id, int> playsThisFrame;

	// Category pools - populated from config
	std::vector<sound::Id> bulletShootIds;
	std::vector<sound::Id> lazerShootIds;
	std::vector<sound::Id> bulletHitIds;
	std::vector<sound::Id> lazerHitIds;
	std::vector<sound::Id> explosionIds;
	std::vector<sound::Id> missileShootIds;
	sound::Id backgroundMusicId = sound::NONE;
	sound::Id alertSoundId = sound::NONE;
	sound::Id thrustSoundId = sound::NONE;

	// Per-category pitch modifiers (virtual ID -> pitch)
	std::map<sound::Id, float> categoryPitches;

	Sound backgroundMusicSound;
	bool musicLoaded = false;

	std::unique_ptr<LoopingFadeSound> thrustSound;

	float masterVolume = 0.5f;
	bool enabled = true;
	bool initialized = false;
	
	int nextSoundId = 1;  // Start at 1, 0 is NONE, negatives are virtual
	std::mt19937 rng{std::random_device{}()};

	void unloadSound(sound::Id id);
	Sound& getNextAlias(sound::Id id);
	void loadFromConfig(const GameConfig& config);
	sound::Id getRandomFromPool(const std::vector<sound::Id>& pool);
	sound::Id resolveVirtualId(sound::Id id);  // Convert virtual IDs to actual random sounds
	bool isExplosionSound(sound::Id id) const;
};

#endif  // SOUND_MANAGER_HPP
