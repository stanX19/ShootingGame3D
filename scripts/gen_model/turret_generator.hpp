#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "gen_types.hpp"

namespace gen_model::turret {

	enum class Archetype {
		BasicShooter,
		HeavyMgRifle,
		LazerDeletor,
		LazerShotgun
	};

	struct Settings {
		std::string id = "basic_shooter";
		Archetype archetype = Archetype::BasicShooter;
		std::uint32_t seed = 4242u;
		float baseRadius = 1.0f; // Unit pivot sphere radius at (0, 0, 0)
		float baseHeight = 0.10f; // Reserved for compatibility
		float barrelLength = 5.20f;
		float barrelRadius = 0.42f;
		int textureWidth = 512;
		int textureHeight = 512;
		bool operator==(const Settings&) const = default;
	};

	struct GenerationReport {
		std::string id;
		Archetype archetype = Archetype::BasicShooter;
		float boundingRadius = 0.0f;
		float boundingHeight = 0.0f;
		std::size_t vertexCount = 0;
		std::size_t triangleCount = 0;
		gen_types::Point3 muzzlePosition{};
		gen_types::Point3 pivotPosition{};
		bool operator==(const GenerationReport&) const = default;
	};

	struct GeneratedTurret {
		gen_types::AssetData asset;
		GenerationReport report;
	};

	GeneratedTurret generate(const Settings& settings);
	Settings defaultBasicShooter();
	Settings defaultHeavyMgRifle();
	Settings defaultLazerDeletor();
	Settings defaultLazerShotgun();

} // namespace gen_model::turret
