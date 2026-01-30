#ifndef COMPONENTS_SOUND_HPP
#define COMPONENTS_SOUND_HPP

#include "sound_manager.hpp"

namespace sound
{
	// Attach to bullets - played when bullet hits something
	// ID is assigned at creation time via soundManager.getRandomX()
	struct HitSound
	{
		Id id = NONE;
		float volume = 1.0f;

		HitSound() = default;
		HitSound(Id _id, float _vol = 1.0f) : id(_id), volume(_vol) {}
	};

	// Attach to weapons - played when weapon fires
	// ID is assigned at creation time via soundManager.getRandomX()
	struct ShootSound
	{
		Id id = NONE;
		float volume = 1.0f;

		ShootSound() = default;
		ShootSound(Id _id, float _vol = 1.0f) : id(_id), volume(_vol) {}
	};

	// Attach to units - played when entity dies
	struct DeathSound
	{
		Id id = NONE;
		float volume = 1.0f;

		DeathSound() = default;
		DeathSound(Id _id, float _vol = 1.0f) : id(_id), volume(_vol) {}
	};

} // namespace sound

#endif // COMPONENTS_SOUND_HPP
