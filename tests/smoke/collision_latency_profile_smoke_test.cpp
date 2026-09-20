#include "catch2/catch_amalgamated.hpp"

#include "game_context.hpp"
#include "systems.hpp"
#include "entities/spaceship_factory.hpp"
#include "components/collision.hpp"
#include "components/physics.hpp"
#include "components/combat.hpp"
#include "components/weapon.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

namespace
{
	const std::vector<GameConfig::RootSource> kSmokeLatencyConfigRoots{
		{"audio", "assets/config/audio.json"},
		{"debug", "assets/config/debug.json"},
		{"game", "assets/config/game.json"},
		{"hud", "assets/config/hud.json"},
		{"loadout", "assets/config/loadout.json"},
		{"physics", "assets/config/physics.json"},
		{"settings", "assets/config/settings.json"},
		{"sounds", "assets/config/sounds.json"},
		{"units", "assets/config/units.json"},
		{"weapons", "assets/config/weapons.json"},
		{"spaceship", "assets/config/spaceships.json"}
	};

	void ensureHeadlessWindow()
	{
		if (!IsWindowReady())
		{
			SetConfigFlags(FLAG_WINDOW_HIDDEN);
			InitWindow(64, 64, "headless_latency_test");
		}
	}
}

TEST_CASE("Smoke test: Collision detection latency profiling with mesh models active", "[smoke][collision][profiling]")
{
	ensureHeadlessWindow();

	GameContext context;
	context.config.init(kSmokeLatencyConfigRoots);

	// Spawn Player Ship with collision model
	spaceship::factory::SpawnParams playerParams;
	playerParams.position = Vector3Zeros;
	playerParams.radius = 3.0f;
	playerParams.faction = faction::FAC_BLUE;
	spaceship::factory::spawnConfiguredSpaceship(context, "player", playerParams);

	// Spawn a fleet of enemy spaceships with collision models
	const std::vector<std::string> enemyShipTypes = {
		"basic", "elite", "fastElite", "heavy_quad", "interceptor_quad", "terminator"
	};

	for (std::size_t i = 0; i < enemyShipTypes.size(); ++i)
	{
		spaceship::factory::SpawnParams enemyParams;
		enemyParams.position = Vector3{
			static_cast<float>(i * 15.0f - 30.0f),
			0.0f,
			static_cast<float>(i * 10.0f + 20.0f)
		};
		enemyParams.radius = 2.5f;
		enemyParams.faction = faction::FAC_RED;
		spaceship::factory::spawnConfiguredSpaceship(context, enemyShipTypes[i], enemyParams);
	}

	// Spawn 30 asteroids surrounding the ships
	for (int i = 0; i < 30; ++i)
	{
		const entt::entity asteroid = context.registry.create();
		context.registry.emplace<physics::Position>(asteroid, Vector3{
			static_cast<float>((i % 6) * 20.0f - 50.0f),
			static_cast<float>((i / 6) * 10.0f - 20.0f),
			static_cast<float>(i * 5.0f - 40.0f)
		});
		context.registry.emplace<physics::Velocity>(asteroid, Vector3{0.5f, 0.0f, 0.5f});
		context.registry.emplace<collision::CollisionBody>(asteroid, 4.0f);
	}

	// Spawn 60 projectiles/bullets
	for (int i = 0; i < 60; ++i)
	{
		const entt::entity bullet = context.registry.create();
		context.registry.emplace<physics::Position>(bullet,
			Vector3{
				static_cast<float>((i % 10) * 10.0f - 45.0f),
				0.0f,
				static_cast<float>(i * 2.0f - 30.0f)
			},
			Vector3{
				static_cast<float>((i % 10) * 10.0f - 45.0f),
				0.0f,
				static_cast<float>(i * 2.0f - 32.0f)
			}
		);
		context.registry.emplace<physics::Velocity>(bullet, Vector3{0.0f, 0.0f, 50.0f});
		context.registry.emplace<collision::CollisionBody>(bullet, 0.3f);
		context.registry.emplace<weapon::tag::Bullet>(bullet);
	}

	const std::size_t totalCollisionBodies = context.registry.storage<collision::CollisionBody>().size();
	const std::size_t totalCollisionModels = context.registry.storage<collision::CollisionBodyModel>().size();

	REQUIRE(totalCollisionBodies > 50);
	REQUIRE(totalCollisionModels >= 7);

	systems::DetectEntityCollision collisionSystem;
	const float dt = 0.016f;

	// Warmup 5 frames
	for (int i = 0; i < 5; ++i)
	{
		collisionSystem.update(context, dt);
		context.dispatcher.update();
	}

	// Profile 100 frames
	constexpr int kProfileFrames = 100;
	std::vector<double> frameTimesUs;
	frameTimesUs.reserve(kProfileFrames);

	for (int frame = 0; frame < kProfileFrames; ++frame)
	{
		const auto startTime = std::chrono::high_resolution_clock::now();
		collisionSystem.update(context, dt);
		const auto endTime = std::chrono::high_resolution_clock::now();

		context.dispatcher.update();

		const double durationUs = std::chrono::duration<double, std::micro>(endTime - startTime).count();
		frameTimesUs.push_back(durationUs);
	}

	const double totalUs = std::accumulate(frameTimesUs.begin(), frameTimesUs.end(), 0.0);
	const double avgUs = totalUs / static_cast<double>(kProfileFrames);
	const double avgMs = avgUs / 1000.0;

	double maxUs = 0.0;
	for (double t : frameTimesUs)
	{
		if (t > maxUs)
			maxUs = t;
	}
	const double maxMs = maxUs / 1000.0;

	std::cout << "\n==========================================================\n";
	std::cout << "     COLLISION DETECTION LATENCY PROFILE (100 FRAMES)     \n";
	std::cout << "==========================================================\n";
	std::cout << " Collision Bodies: " << totalCollisionBodies << '\n';
	std::cout << " Mesh Models:     " << totalCollisionModels << '\n';
	std::cout << " Average Latency: " << std::fixed << std::setprecision(3) << avgMs << " ms (" << avgUs << " us)\n";
	std::cout << " Max Latency:     " << std::fixed << std::setprecision(3) << maxMs << " ms (" << maxUs << " us)\n";
	std::cout << "==========================================================\n\n";

	// Real-time latency budget assertion:
	// A 60 FPS frame has 16.6ms total budget for ALL systems.
	// In unoptimized headless debug test builds with 7 detailed BVH meshes and ~150 entities, average should be under 25.0 ms.
	CHECK(avgMs < 25.0);
	CHECK(maxMs < 50.0);
}
