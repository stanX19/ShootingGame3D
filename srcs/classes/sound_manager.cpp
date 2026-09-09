#include "sound_manager.hpp"
#include "game_config.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>

using json = nlohmann::json;

SoundManager::SoundManager() {}

SoundManager::~SoundManager()
{
	shutdown();
}

void SoundManager::loadFromConfig(const GameConfig &config)
{
	json soundsSection = config.getSection("sounds");
	if (soundsSection.empty())
	{
		TraceLog(LOG_WARNING, "SOUND: No sounds section in config");
		return;
	}

	// Load background music
	if (soundsSection.contains("backgroundMusic"))
	{
		std::string path = soundsSection["backgroundMusic"].get<std::string>();
		if (path != "none")
		{
			m_backgroundMusicSound = LoadSound(path.c_str());
			m_musicLoaded = IsSoundValid(m_backgroundMusicSound);
			if (m_musicLoaded)
			{
				m_backgroundMusicId = m_nextSoundId++;
			}
		}
	}

	// Helper lambda to load a category
	auto loadCategory = [&](const std::string &key, std::vector<sound::Id> &pool)
	{
		if (!soundsSection.contains(key))
			return;
		for (const auto &path : soundsSection[key])
		{
			sound::Id id = loadSound(path.get<std::string>());
			if (id != sound::NONE)
			{
				pool.push_back(id);
			}
		}
	};

	loadCategory("bulletShoot", m_bulletShootIds);
	loadCategory("lazerShoot", m_lazerShootIds);
	loadCategory("bulletHit", m_bulletHitIds);
	loadCategory("lazerHit", m_lazerHitIds);
	loadCategory("explosion", m_explosionIds);
	loadCategory("missileShoot", m_missileShootIds);

	// Load per-category pitch modifiers (for "space cockpit" sound effect)
	auto loadPitch = [&](const std::string &key, sound::Id virtualId)
	{
		if (soundsSection.contains(key))
		{
			m_categoryPitches[virtualId] = soundsSection[key].get<float>();
		}
	};
	loadPitch("bulletShootPitch", sound::RANDOM_BULLET_SHOOT);
	loadPitch("lazerShootPitch", sound::RANDOM_LAZER_SHOOT);
	loadPitch("bulletHitPitch", sound::RANDOM_BULLET_HIT);
	loadPitch("lazerHitPitch", sound::RANDOM_LAZER_HIT);
	loadPitch("explosionPitch", sound::RANDOM_EXPLOSION);
	loadPitch("missileShootPitch", sound::RANDOM_MISSILE_SHOOT);

	// Load alert sound
	if (soundsSection.contains("alert"))
	{
		std::string alertPath = soundsSection["alert"].get<std::string>();
		m_alertSoundId = loadSound(alertPath);
	}

	// Load thrust sound
	if (soundsSection.contains("thrust"))
	{
		std::string path = soundsSection["thrust"].get<std::string>();
		float threshold = 2.0f;
		if (soundsSection.contains("thrustRestartThreshold"))
		{
			threshold = soundsSection["thrustRestartThreshold"].get<float>();
		}
		m_thrustSound = std::make_unique<LoopingFadeSound>(path, threshold);
		m_thrustSound->init(m_masterVolume);
		if (soundsSection.contains("thrustFadeIn"))
		{
			m_thrustSound->setFadeInDuration(soundsSection["thrustFadeIn"].get<float>());
		}
		if (soundsSection.contains("thrustFadeOut"))
		{
			m_thrustSound->setFadeOutDuration(soundsSection["thrustFadeOut"].get<float>());
		}
		if (soundsSection.contains("thrustVolume"))
		{
			m_thrustSound->setMaxVolume(soundsSection["thrustVolume"].get<float>());
		}
	}

	TraceLog(LOG_INFO, "SOUND: Loaded %zu bulletShoot, %zu lazerShoot, %zu bulletHit, %zu lazerHit, %zu explosion sounds",
			 m_bulletShootIds.size(), m_lazerShootIds.size(), m_bulletHitIds.size(), m_lazerHitIds.size(), m_explosionIds.size());
}

void SoundManager::init(const GameConfig &config)
{
	if (m_initialized)
		return;

	m_masterVolume = config.getFloat("audio.masterVolume", 0.5f);
	m_maxSoundsPerIdPerFrame = config.getInt("audio.maxSoundsPerIdPerFrame", 3);
	m_soundAliasesCount = config.getInt("audio.soundAliasCount", 4);
	m_explosionMaxDistance = config.getFloat("audio.explosionMaxDistance", -1.0f);

	InitAudioDevice();
	if (!IsAudioDeviceReady())
	{
		TraceLog(LOG_WARNING, "SOUND: Failed to initialize audio device");
		m_enabled = false;
		return;
	}

	m_initialized = true;
	loadFromConfig(config);

	SetMasterVolume(m_masterVolume);
	TraceLog(LOG_INFO, "SOUND: Audio system initialized");
}

void SoundManager::shutdown()
{
	if (!m_initialized)
		return;

	if (m_musicLoaded)
	{
		StopSound(m_backgroundMusicSound);
		UnloadSound(m_backgroundMusicSound);
		m_musicLoaded = false;
	}

	if (m_thrustSound)
	{
		m_thrustSound->shutdown();
		m_thrustSound.reset();
	}

	for (auto &[id, data] : m_sounds)
	{
		unloadSound(id);
	}
	m_sounds.clear();

	// Clear pools
	m_bulletShootIds.clear();
	m_lazerShootIds.clear();
	m_bulletHitIds.clear();
	m_lazerHitIds.clear();
	m_explosionIds.clear();
	m_missileShootIds.clear();

	CloseAudioDevice();
	m_initialized = false;
}

sound::Id SoundManager::loadSound(const GameConfig &config, const std::string &configPath, sound::Id defaultId)
{
	std::string path = config.getString(configPath, "");
	if (path.empty())
		return defaultId;
	return loadSound(path);
}

sound::Id SoundManager::loadSound(const std::string &path)
{
	if (!m_initialized || !m_enabled)
		return sound::NONE;

	// Check cache first
	auto cacheIt = m_pathCache.find(path);
	if (cacheIt != m_pathCache.end())
	{
		return cacheIt->second;
	}

	Sound baseSound = LoadSound(path.c_str());
	if (!IsSoundValid(baseSound))
	{
		TraceLog(LOG_WARNING, "SOUND: Failed to load sound: %s", path.c_str());
		return sound::NONE;
	}

	sound::Id id = m_nextSoundId++;
	SoundData data;
	data.baseSound = baseSound;
	data.loaded = true;

	// Create aliases for simultaneous playback
	for (int i = 0; i < m_soundAliasesCount; i++)
	{
		data.aliases.push_back(LoadSoundAlias(baseSound));
	}

	m_sounds[id] = std::move(data);
	m_pathCache[path] = id;
	return id;
}

void SoundManager::unloadSound(sound::Id id)
{
	auto it = m_sounds.find(id);
	if (it == m_sounds.end())
		return;

	for (auto &alias : it->second.aliases)
	{
		UnloadSoundAlias(alias);
	}
	UnloadSound(it->second.baseSound);
}

Sound &SoundManager::getNextAlias(sound::Id id)
{
	auto &data = m_sounds[id];
	int idx = data.currentAlias;
	data.currentAlias = (data.currentAlias + 1) % static_cast<int>(data.aliases.size());
	return data.aliases[idx];
}

sound::Id SoundManager::getRandomFromPool(const std::vector<sound::Id> &pool)
{
	if (pool.empty())
		return sound::NONE;
	std::uniform_int_distribution<size_t> dist(0, pool.size() - 1);
	return pool[dist(m_rng)];
}

sound::Id SoundManager::getRandomBulletShoot()
{
	return getRandomFromPool(m_bulletShootIds);
}

sound::Id SoundManager::getRandomLazerShoot()
{
	return getRandomFromPool(m_lazerShootIds);
}

sound::Id SoundManager::getRandomBulletHit()
{
	return getRandomFromPool(m_bulletHitIds);
}

sound::Id SoundManager::getRandomLazerHit()
{
	return getRandomFromPool(m_lazerHitIds);
}

sound::Id SoundManager::getRandomExplosion()
{
	return getRandomFromPool(m_explosionIds);
}

sound::Id SoundManager::getRandomMissileShoot()
{
	return getRandomFromPool(m_missileShootIds);
}

sound::Id SoundManager::resolveVirtualId(sound::Id id)
{
	switch (id)
	{
	case sound::RANDOM_BULLET_SHOOT:
		return getRandomFromPool(m_bulletShootIds);
	case sound::RANDOM_LAZER_SHOOT:
		return getRandomFromPool(m_lazerShootIds);
	case sound::RANDOM_BULLET_HIT:
		return getRandomFromPool(m_bulletHitIds);
	case sound::RANDOM_LAZER_HIT:
		return getRandomFromPool(m_lazerHitIds);
	case sound::RANDOM_EXPLOSION:
		return getRandomFromPool(m_explosionIds);
	case sound::RANDOM_MISSILE_SHOOT:
		return getRandomFromPool(m_missileShootIds);
	default:
		return id; // Not virtual, return as-is
	}
}

sound::Id SoundManager::getBackgroundMusic() const
{
	return m_backgroundMusicId;
}

sound::Id SoundManager::getAlertSound() const
{
	return m_alertSoundId;
}

sound::Id SoundManager::getThrustSound() const
{
	return m_thrustSoundId;
}

void SoundManager::queueSound(const GameConfig &config, const std::string &configPath, Vector3 position, float volume)
{
	queueSound(loadSound(config, configPath), position, volume);
}

void SoundManager::queueSound(sound::Id id, Vector3 position, float volume)
{
	if (!m_enabled || id == sound::NONE)
		return;

	// Resolve virtual IDs to actual sounds at queue time
	sound::Id resolved = resolveVirtualId(id);
	if (resolved == sound::NONE)
		return;

	m_pendingRequests.push_back({resolved, position, volume});
}

void SoundManager::update(const Camera3D &camera)
{
	if (!m_initialized || !m_enabled)
	{
		m_pendingRequests.clear();
		return;
	}

	// Reset per-frame counters
	m_playsThisFrame.clear();

	for (const auto &req : m_pendingRequests)
	{
		// Throttle: skip if we've played too many of this sound this frame
		if (m_playsThisFrame[req.id] >= m_maxSoundsPerIdPerFrame)
		{
			continue;
		}

		// Check if sound is loaded
		auto it = m_sounds.find(req.id);
		if (it == m_sounds.end() || !it->second.loaded)
			continue;

		// Calculate volume based on distance (simple falloff)
		float distance = Vector3Distance(camera.position, req.position);

		// Skip explosion sounds beyond max distance
		if (m_explosionMaxDistance > 0 && isExplosionSound(req.id) && distance > m_explosionMaxDistance)
		{
			continue;
		}
		float distanceAttenuation = 1.0f / (1.0f + distance * 0.01f); // gentle falloff
		distanceAttenuation = std::max(0.1f, std::min(1.0f, distanceAttenuation));

		float finalVolume = req.volume * distanceAttenuation * m_masterVolume;

		// Calculate pan based on position relative to camera
		Vector3 toSound = req.position - camera.position;
		Vector3 cameraRight = Vector3CrossProduct(camera.up, camera.target - camera.position);
		cameraRight = Vector3Normalize(cameraRight);
		float pan = Vector3DotProduct(toSound, cameraRight);
		pan = std::max(-1.0f, std::min(1.0f, pan * 0.01f)); // normalize
		pan = 0.5f + pan * 0.4f;							// convert to 0.1-0.9 range (centered at 0.5)

		// Determine pitch from category (default 1.0f)
		float pitch = 1.0f;
		for (const auto &[virtualId, p] : m_categoryPitches)
		{
			if (virtualId == sound::RANDOM_BULLET_SHOOT && std::find(m_bulletShootIds.begin(), m_bulletShootIds.end(), req.id) != m_bulletShootIds.end())
			{
				pitch = p;
				break;
			}
			if (virtualId == sound::RANDOM_LAZER_SHOOT && std::find(m_lazerShootIds.begin(), m_lazerShootIds.end(), req.id) != m_lazerShootIds.end())
			{
				pitch = p;
				break;
			}
			if (virtualId == sound::RANDOM_BULLET_HIT && std::find(m_bulletHitIds.begin(), m_bulletHitIds.end(), req.id) != m_bulletHitIds.end())
			{
				pitch = p;
				break;
			}
			if (virtualId == sound::RANDOM_LAZER_HIT && std::find(m_lazerHitIds.begin(), m_lazerHitIds.end(), req.id) != m_lazerHitIds.end())
			{
				pitch = p;
				break;
			}
			if (virtualId == sound::RANDOM_EXPLOSION && std::find(m_explosionIds.begin(), m_explosionIds.end(), req.id) != m_explosionIds.end())
			{
				pitch = p;
				break;
			}
			if (virtualId == sound::RANDOM_MISSILE_SHOOT && std::find(m_missileShootIds.begin(), m_missileShootIds.end(), req.id) != m_missileShootIds.end())
			{
				pitch = p;
				break;
			}
		}

		Sound &alias = getNextAlias(req.id);
		SetSoundVolume(alias, finalVolume);
		SetSoundPitch(alias, pitch);
		SetSoundPan(alias, pan);
		PlaySound(alias);

		m_playsThisFrame[req.id]++;
	}

	m_pendingRequests.clear();
}

void SoundManager::playImmediate(sound::Id id, float volume)
{
	if (!m_initialized || !m_enabled)
		return;

	// Resolve virtual IDs to actual sounds
	sound::Id resolved = resolveVirtualId(id);
	if (resolved == sound::NONE)
		return;

	auto it = m_sounds.find(resolved);
	if (it == m_sounds.end() || !it->second.loaded)
		return;

	Sound &alias = getNextAlias(resolved);
	SetSoundVolume(alias, volume * m_masterVolume);
	SetSoundPan(alias, 0.5f);
	PlaySound(alias);
}

void SoundManager::playImmediate(const GameConfig &config, const std::string &configPath, float volume)
{
	playImmediate(loadSound(config, configPath), volume);
}

void SoundManager::playMusic()
{
	if (!m_initialized || !m_enabled || !m_musicLoaded)
		return;
	SetSoundVolume(m_backgroundMusicSound, m_masterVolume);
	PlaySound(m_backgroundMusicSound);
}

void SoundManager::updateMusic()
{
	if (!m_initialized || !m_enabled || !m_musicLoaded)
		return;
	// Loop the music by replaying when it finishes
	if (!IsSoundPlaying(m_backgroundMusicSound))
	{
		PlaySound(m_backgroundMusicSound);
	}
}

void SoundManager::stopMusic()
{
	if (!m_musicLoaded)
		return;
	StopSound(m_backgroundMusicSound);
}

void SoundManager::updateThrustSound(bool isAccelerating, float dt)
{
	if (!m_initialized || !m_enabled || !m_thrustSound)
		return;
	m_thrustSound->update(isAccelerating, dt, m_masterVolume);
}

void SoundManager::setMasterVolume(float volume)
{
	m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
	if (m_initialized)
	{
		SetMasterVolume(m_masterVolume);
	}
}

float SoundManager::getMasterVolume() const
{
	return m_masterVolume;
}

void SoundManager::setEnabled(bool value)
{
	m_enabled = value;
}

bool SoundManager::isEnabled() const
{
	return m_enabled;
}

bool SoundManager::isExplosionSound(sound::Id id) const
{
	return std::find(m_explosionIds.begin(), m_explosionIds.end(), id) != m_explosionIds.end();
}
