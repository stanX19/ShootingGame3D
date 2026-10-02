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

struct TrailNode {
	Vector3 pos = Vector3Zeros;
	float alpha = 1.0f;
	float age = 0.0f;
};

struct HasSimpleTrail {
	static constexpr std::size_t MAX_NODES = 8;

	TrailNode nodes[MAX_NODES]{};
	std::uint8_t count = 0;
	std::uint8_t maxNodes = 2; // 2 for Bullet Tracers, 8 for Missiles

	float maxAge = 0.05f;       // Lifetime of nodes in seconds
	float minDistance = 1.0f;   // Distance threshold before dropping next breadcrumb
	float width = 0.15f;        // Width at head
	float endWidth = 0.0f;      // Width at tail
	Color color = GRAY;
	Vector3 lastRecordedPos = Vector3Zeros;
	bool hasLastRecordedPos = false;
};

struct HasMultiTrail {
	static constexpr std::size_t MAX_NODES = 10;
	static constexpr std::size_t MAX_EMITTERS = 8;

	struct Emitter {
		TrailNode nodes[MAX_NODES]{};
		std::uint8_t count = 0;
		Vector3 localOffset = Vector3Zeros; // Nozzle offset in local space
		float width = 0.3f;                 // Nozzle-specific width
		Vector3 lastRecordedPos = Vector3Zeros;
		bool hasLastRecordedPos = false;
	};

	Emitter emitters[MAX_EMITTERS]{};
	std::uint8_t emitterCount = 0;
	std::uint8_t maxNodes = 10;

	float maxAge = 0.35f;       // Lifetime of nodes in seconds
	float minDistance = 1.5f;   // Distance threshold before dropping next breadcrumb
	float endWidth = 0.0f;      // Width at tail
	Color color = SKYBLUE;
};

using HasTrails = HasSimpleTrail;

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
struct PostDeath {};

} // namespace tag

} // namespace effect

namespace tag {
using ::effect::tag::DropDebris;
using ::effect::tag::PostDeath;
namespace effect = ::effect::tag;
} // namespace tag

using ::effect::DEFAULT_EXPLOSION_DURATION;
using ::effect::DEFAULT_EXPLOSION_DAMAGE;
using ::effect::EXPLOSION_COLOR;
using ::effect::ExplodeOnDeath;
using ::effect::InstantDamageOnDeath;
using ::effect::SpawnsTrailParticles;
using ::effect::Trail;
using ::effect::TrailNode;
using ::effect::HasSimpleTrail;
using ::effect::HasMultiTrail;
using ::effect::HasTrails;

#endif // COMPONENTS_EFFECT_HPP
