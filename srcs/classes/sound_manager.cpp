#include "sound_manager.hpp"
#include "game_config.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>

using json = nlohmann::json;

SoundManager::SoundManager() {}

SoundManager::~SoundManager() {
	shutdown();
}

void SoundManager::loadFromConfig(const GameConfig& config) {
	json soundsSection = config.getSection("sounds");
	if (soundsSection.empty()) {
		TraceLog(LOG_WARNING, "SOUND: No sounds section in config");
		return;
	}

	// Load background music
	if (soundsSection.contains("backgroundMusic")) {
		std::string path = soundsSection["backgroundMusic"].get<std::string>();
		backgroundMusic = LoadMusicStream(path.c_str());
		musicLoaded = IsMusicValid(backgroundMusic);
		if (musicLoaded) {
			backgroundMusic.looping = true;
			backgroundMusicId = nextSoundId++;
		}
	}

	// Helper lambda to load a category
	auto loadCategory = [&](const std::string& key, std::vector<sound::Id>& pool) {
		if (!soundsSection.contains(key)) return;
		for (const auto& path : soundsSection[key]) {
			sound::Id id = loadSound(path.get<std::string>());
			if (id != sound::NONE) {
				pool.push_back(id);
			}
		}
	};

	loadCategory("bulletShoot", bulletShootIds);
	loadCategory("lazerShoot", lazerShootIds);
	loadCategory("bulletHit", bulletHitIds);
	loadCategory("lazerHit", lazerHitIds);
	loadCategory("explosion", explosionIds);

	TraceLog(LOG_INFO, "SOUND: Loaded %zu bulletShoot, %zu lazerShoot, %zu bulletHit, %zu lazerHit, %zu explosion sounds",
		bulletShootIds.size(), lazerShootIds.size(), bulletHitIds.size(), lazerHitIds.size(), explosionIds.size());
}

void SoundManager::init(const GameConfig& config) {
	if (initialized) return;

	int bufferSize = config.getInt("audio.bufferSize", 65536);
	masterVolume = config.getFloat("audio.masterVolume", 0.5f);
	maxSoundsPerIdPerFrame = config.getInt("audio.maxSoundsPerIdPerFrame", 3);
	soundAliasesCount = config.getInt("audio.soundAliasCount", 4);

	SetAudioStreamBufferSizeDefault(bufferSize);
	InitAudioDevice();
	if (!IsAudioDeviceReady()) {
		TraceLog(LOG_WARNING, "SOUND: Failed to initialize audio device");
		enabled = false;
		return;
	}

	initialized = true;
	loadFromConfig(config);

	SetMasterVolume(masterVolume);
	TraceLog(LOG_INFO, "SOUND: Audio system initialized (bufferSize=%d)", bufferSize);
}

void SoundManager::shutdown() {
	if (!initialized) return;

	if (musicLoaded) {
		StopMusicStream(backgroundMusic);
		UnloadMusicStream(backgroundMusic);
		musicLoaded = false;
	}

	for (auto& [id, data] : sounds) {
		unloadSound(id);
	}
	sounds.clear();

	// Clear pools
	bulletShootIds.clear();
	lazerShootIds.clear();
	bulletHitIds.clear();
	lazerHitIds.clear();
	explosionIds.clear();

	CloseAudioDevice();
	initialized = false;
}

sound::Id SoundManager::loadSound(const std::string& path) {
	if (!initialized || !enabled) return sound::NONE;

	Sound baseSound = LoadSound(path.c_str());
	if (!IsSoundValid(baseSound)) {
		TraceLog(LOG_WARNING, "SOUND: Failed to load sound: %s", path.c_str());
		return sound::NONE;
	}

	sound::Id id = nextSoundId++;
	SoundData data;
	data.baseSound = baseSound;
	data.loaded = true;

	// Create aliases for simultaneous playback
	for (int i = 0; i < soundAliasesCount; i++) {
		data.aliases.push_back(LoadSoundAlias(baseSound));
	}

	sounds[id] = std::move(data);
	return id;
}

void SoundManager::unloadSound(sound::Id id) {
	auto it = sounds.find(id);
	if (it == sounds.end()) return;

	for (auto& alias : it->second.aliases) {
		UnloadSoundAlias(alias);
	}
	UnloadSound(it->second.baseSound);
}

Sound& SoundManager::getNextAlias(sound::Id id) {
	auto& data = sounds[id];
	int idx = data.currentAlias;
	data.currentAlias = (data.currentAlias + 1) % static_cast<int>(data.aliases.size());
	return data.aliases[idx];
}

sound::Id SoundManager::getRandomFromPool(const std::vector<sound::Id>& pool) {
	if (pool.empty()) return sound::NONE;
	std::uniform_int_distribution<size_t> dist(0, pool.size() - 1);
	return pool[dist(rng)];
}

sound::Id SoundManager::getRandomBulletShoot() {
	return getRandomFromPool(bulletShootIds);
}

sound::Id SoundManager::getRandomLazerShoot() {
	return getRandomFromPool(lazerShootIds);
}

sound::Id SoundManager::getRandomBulletHit() {
	return getRandomFromPool(bulletHitIds);
}

sound::Id SoundManager::getRandomLazerHit() {
	return getRandomFromPool(lazerHitIds);
}

sound::Id SoundManager::getRandomExplosion() {
	return getRandomFromPool(explosionIds);
}

sound::Id SoundManager::resolveVirtualId(sound::Id id) {
	switch (id) {
		case sound::RANDOM_BULLET_SHOOT: return getRandomFromPool(bulletShootIds);
		case sound::RANDOM_LAZER_SHOOT:  return getRandomFromPool(lazerShootIds);
		case sound::RANDOM_BULLET_HIT:   return getRandomFromPool(bulletHitIds);
		case sound::RANDOM_LAZER_HIT:    return getRandomFromPool(lazerHitIds);
		case sound::RANDOM_EXPLOSION:    return getRandomFromPool(explosionIds);
		default: return id;  // Not virtual, return as-is
	}
}

sound::Id SoundManager::getBackgroundMusic() const {
	return backgroundMusicId;
}

void SoundManager::queueSound(sound::Id id, Vector3 position, float volume) {
	if (!enabled || id == sound::NONE) return;

	// Resolve virtual IDs to actual sounds at queue time
	sound::Id resolved = resolveVirtualId(id);
	if (resolved == sound::NONE) return;

	pendingRequests.push_back({resolved, position, volume});
}

void SoundManager::update(const Camera3D& camera) {
	if (!initialized || !enabled) {
		pendingRequests.clear();
		return;
	}

	// Reset per-frame counters
	playsThisFrame.clear();

	for (const auto& req : pendingRequests) {
		// Throttle: skip if we've played too many of this sound this frame
		if (playsThisFrame[req.id] >= maxSoundsPerIdPerFrame) {
			continue;
		}

		// Check if sound is loaded
		auto it = sounds.find(req.id);
		if (it == sounds.end() || !it->second.loaded) continue;

		// Calculate volume based on distance (simple falloff)
		float distance = Vector3Distance(camera.position, req.position);
		float distanceAttenuation = 1.0f / (1.0f + distance * 0.01f);  // gentle falloff
		distanceAttenuation = std::max(0.1f, std::min(1.0f, distanceAttenuation));

		float finalVolume = req.volume * distanceAttenuation * masterVolume;

		// Calculate pan based on position relative to camera
		Vector3 toSound = Vector3Subtract(req.position, camera.position);
		Vector3 cameraRight = Vector3CrossProduct(camera.up, 
			Vector3Subtract(camera.target, camera.position));
		cameraRight = Vector3Normalize(cameraRight);
		float pan = Vector3DotProduct(toSound, cameraRight);
		pan = std::max(-1.0f, std::min(1.0f, pan * 0.01f));  // normalize
		pan = 0.5f + pan * 0.4f;  // convert to 0.1-0.9 range (centered at 0.5)

		Sound& alias = getNextAlias(req.id);
		SetSoundVolume(alias, finalVolume);
		SetSoundPan(alias, pan);
		PlaySound(alias);

		playsThisFrame[req.id]++;
	}

	pendingRequests.clear();
}

void SoundManager::playImmediate(sound::Id id, float volume) {
	if (!initialized || !enabled) return;

	// Resolve virtual IDs to actual sounds
	sound::Id resolved = resolveVirtualId(id);
	if (resolved == sound::NONE) return;

	auto it = sounds.find(resolved);
	if (it == sounds.end() || !it->second.loaded) return;

	Sound& alias = getNextAlias(resolved);
	SetSoundVolume(alias, volume * masterVolume);
	SetSoundPan(alias, 0.5f);
	PlaySound(alias);
}

void SoundManager::playMusic() {
	if (!initialized || !enabled || !musicLoaded) return;
	PlayMusicStream(backgroundMusic);
}

void SoundManager::updateMusic() {
	if (!initialized || !enabled || !musicLoaded)
		return;
	UpdateMusicStream(backgroundMusic);
}

void SoundManager::stopMusic() {
	if (!musicLoaded) return;
	StopMusicStream(backgroundMusic);
}

void SoundManager::setMasterVolume(float volume) {
	masterVolume = std::max(0.0f, std::min(1.0f, volume));
	if (initialized) {
		SetMasterVolume(masterVolume);
	}
}

float SoundManager::getMasterVolume() const {
	return masterVolume;
}

void SoundManager::setEnabled(bool value) {
	enabled = value;
}

bool SoundManager::isEnabled() const {
	return enabled;
}
