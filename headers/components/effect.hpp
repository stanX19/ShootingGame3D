#ifndef COMPONENTS_EFFECT_HPP
#define COMPONENTS_EFFECT_HPP

#include "includes.hpp"
#include <cstddef>
#include <cstdint>

namespace effect {

inline constexpr float DEFAULT_EXPLOSION_DURATION = 10.0f;
inline constexpr float DEFAULT_EXPLOSION_DAMAGE = 50.0f;
inline const Color EXPLOSION_COLOR = ORANGE;

struct ExplodeOnDeath
{
	float finalRadius = 0.0f;
	float startRadius = 0.0f;
	float explosionDuration = DEFAULT_EXPLOSION_DURATION;
	float damage = DEFAULT_EXPLOSION_DAMAGE;
	Color color = EXPLOSION_COLOR;

	static ExplodeOnDeath createFromRadDmg(float parentRad, float parentDmg)
	{
		return ExplodeOnDeath{
			parentRad * 10.0f,
			parentRad * 0.7f,
			DEFAULT_EXPLOSION_DURATION,
			parentDmg,
			EXPLOSION_COLOR
		};
	}

	static ExplodeOnDeath createFromStartEndRad(float startRad, float endRad, float damage) {
		return ExplodeOnDeath{
			endRad,
			startRad,
			DEFAULT_EXPLOSION_DURATION,
			damage,
			EXPLOSION_COLOR
		};
	}
};

struct InstantDamageOnDeath
{
	float instantDamage = 0.0f;
	float radius = 0.0f;
};

struct SpawnsTrailParticles {
	static constexpr std::size_t maxSpawnLocations = 8;

	Vector3 spawnLocations[maxSpawnLocations]{};
	std::uint8_t spawnCount = 0;
	float radius = 0.5f;
	float lifespan = 1.0f;
	Color color = SKYBLUE;
};

struct Trail {
	Color color = WHITE;
	float rad = 0.1f;
	Trail(Color c = WHITE, float r = 0.1f): color(c), rad(r) {}
};

namespace tag {

struct DropDebris {};

} // namespace tag

} // namespace effect

namespace tag {
using ::effect::tag::DropDebris;
namespace effect = ::effect::tag;
} // namespace tag

using ::effect::DEFAULT_EXPLOSION_DURATION;
using ::effect::DEFAULT_EXPLOSION_DAMAGE;
using ::effect::EXPLOSION_COLOR;
using ::effect::ExplodeOnDeath;
using ::effect::InstantDamageOnDeath;
using ::effect::SpawnsTrailParticles;
using ::effect::Trail;

#endif // COMPONENTS_EFFECT_HPP
