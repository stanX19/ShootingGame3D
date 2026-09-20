#include "game_context.hpp"
#include "systems.hpp"
#include "entities/spaceship_factory.hpp"
#include "entities.hpp"
#include "components/collision.hpp"
#include "components/physics.hpp"
#include "components/combat.hpp"
#include "components/weapon.hpp"
#include "collision_algorithm.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

namespace
{
	const std::vector<GameConfig::RootSource> kConfigRoots{
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

	void ensureHeadless()
	{
		if (!IsWindowReady())
		{
			SetConfigFlags(FLAG_WINDOW_HIDDEN);
			InitWindow(64, 64, "profile_mesh_mesh");
		}
	}

	struct LatencyStats
	{
		double avgUs = 0.0;
		double minUs = 0.0;
		double maxUs = 0.0;
		double p95Us = 0.0;
	};

	LatencyStats computeStats(std::vector<double> &samples)
	{
		if (samples.empty()) return {};
		std::sort(samples.begin(), samples.end());
		double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
		LatencyStats stats;
		stats.avgUs = sum / samples.size();
		stats.minUs = samples.front();
		stats.maxUs = samples.back();
		size_t p95Idx = static_cast<size_t>(samples.size() * 0.95);
		stats.p95Us = samples[std::min(p95Idx, samples.size() - 1)];
		return stats;
	}

	void printStats(const std::string &name, const LatencyStats &stats)
	{
		std::cout << std::left << std::setw(38) << name
		          << " | Avg: " << std::fixed << std::setprecision(2) << std::setw(8) << stats.avgUs << " us ("
		          << std::setw(6) << (stats.avgUs / 1000.0) << " ms)"
		          << " | Max: " << std::setw(8) << stats.maxUs << " us ("
		          << std::setw(6) << (stats.maxUs / 1000.0) << " ms)"
		          << " | Min: " << std::setw(8) << stats.minUs << " us"
		          << " | P95: " << std::setw(8) << stats.p95Us << " us"
		          << std::endl;
	}

	struct EntityData {
		entt::entity id;
		Vector3 pos;
		Vector3 vel;
		float rad;
		int faction;
		entt::entity assemblyRoot;
		const CollisionBodyModel *collisionBodyModel;
		const RenderBody *renderBody;
	};

	struct MeshCollisionResult {
		bool usesMeshNarrowPhase;
		std::optional<CollisionHit> hit;
	};

	bool usesMeshCollision(const EntityData &entity)
	{
		return entity.collisionBodyModel != nullptr && entity.renderBody != nullptr;
	}

	MeshCollisionResult processMeshCollision(
		const GameContext &context,
		const EntityData &A,
		const EntityData &B,
		const CollisionInterval &interval
	)
	{
		const bool AUsesMesh = usesMeshCollision(A);
		const bool BUsesMesh = usesMeshCollision(B);
		if (!AUsesMesh && !BUsesMesh)
		{
			return MeshCollisionResult{false, std::nullopt};
		}

		const EntityData *meshEntity = nullptr;
		const EntityData *sphereEntity = nullptr;
		if (AUsesMesh && BUsesMesh)
		{
			meshEntity = (A.rad >= B.rad) ? &A : &B;
			sphereEntity = (A.rad >= B.rad) ? &B : &A;
		}
		else
		{
			meshEntity = AUsesMesh ? &A : &B;
			sphereEntity = AUsesMesh ? &B : &A;
		}
		const CollisionModel &collisionModel = context.collisionBodyManager.getCollisionModel(
			meshEntity->collisionBodyModel->modelID
		);
		const CollisionMeshInstance meshInstance{
			meshEntity->pos,
			meshEntity->pos + meshEntity->vel,
			meshEntity->renderBody->translation,
			meshEntity->renderBody->scale,
			meshEntity->renderBody->rotation
		};
		const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
			collisionModel,
			meshInstance,
			sphereEntity->pos,
			sphereEntity->vel,
			sphereEntity->rad,
			interval
		);
		return MeshCollisionResult{true, hit};
	}
}

int main()
{
	ensureHeadless();

	std::cout << "========================================================================\n";
	std::cout << "           MESH-MESH COLLISION LATENCY PROFILING SUITE                  \n";
	std::cout << "========================================================================\n";

	constexpr int kIterations = 200;

	// -------------------------------------------------------------------------
	// Scenario 1: Direct sweepSphereAgainstMesh microbenchmark (Ship sphere vs Asteroid mesh)
	// -------------------------------------------------------------------------
	{
		GameContext context;
		context.config.init(kConfigRoots);

		t_model_id asteroidModel = context.modelManager.loadModel("assets/Models/asteroid/asteroid_big.obj");
		t_collision_mesh_id asteroidMeshId = context.collisionBodyManager.loadCollisionModel(context, asteroidModel);
		const CollisionModel &collisionModel = context.collisionBodyManager.getCollisionModel(asteroidMeshId);

		const float asteroidRad = 150.0f;
		CollisionMeshInstance meshInstance{
			Vector3Zeros,
			Vector3Zeros,
			Vector3Zeros,
			Vector3{asteroidRad, asteroidRad, asteroidRad},
			QuaternionIdentity()
		};

		// Test A: Sphere completely outside asteroid bounds
		std::vector<double> outsideSamples;
		outsideSamples.reserve(kIterations);
		for (int i = 0; i < kIterations; ++i)
		{
			Vector3 spherePrev{0.0f, 0.0f, 300.0f};
			Vector3 sphereVel{0.0f, 0.0f, -10.0f};
			CollisionInterval interval{0.0f, 1.0f};

			auto t0 = std::chrono::high_resolution_clock::now();
			auto hit = sweepSphereAgainstMesh(collisionModel, meshInstance, spherePrev, sphereVel, 4.0f, interval);
			auto t1 = std::chrono::high_resolution_clock::now();
			outsideSamples.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
			(void)hit;
		}
		printStats("1A. Ship vs Asteroid (outside bounds)", computeStats(outsideSamples));

		// Test B: Sphere inside asteroid AABB bounds, approaching surface (triggers isPointInsideMesh!)
		std::vector<double> nearSurfaceSamples;
		nearSurfaceSamples.reserve(kIterations);
		for (int i = 0; i < kIterations; ++i)
		{
			Vector3 spherePrev{0.0f, 0.0f, 120.0f};
			Vector3 sphereVel{0.0f, 0.0f, -5.0f};
			CollisionInterval interval{0.0f, 1.0f};

			auto t0 = std::chrono::high_resolution_clock::now();
			auto hit = sweepSphereAgainstMesh(collisionModel, meshInstance, spherePrev, sphereVel, 4.0f, interval);
			auto t1 = std::chrono::high_resolution_clock::now();
			nearSurfaceSamples.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
			(void)hit;
		}
		printStats("1B. Ship vs Asteroid (inside AABB, near surface)", computeStats(nearSurfaceSamples));

		// Test C: Sphere actually colliding with asteroid surface triangles
		std::vector<double> collidingSamples;
		collidingSamples.reserve(kIterations);
		for (int i = 0; i < kIterations; ++i)
		{
			Vector3 spherePrev{0.0f, 0.0f, 100.0f};
			Vector3 sphereVel{0.0f, 0.0f, -20.0f};
			CollisionInterval interval{0.0f, 1.0f};

			auto t0 = std::chrono::high_resolution_clock::now();
			auto hit = sweepSphereAgainstMesh(collisionModel, meshInstance, spherePrev, sphereVel, 4.0f, interval);
			auto t1 = std::chrono::high_resolution_clock::now();
			collidingSamples.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
			(void)hit;
		}
		printStats("1C. Ship vs Asteroid (surface collision)", computeStats(collidingSamples));
	}

	// -------------------------------------------------------------------------
	// Scenario 2: Microbenchmark on Capital Ships (Terminator: 4640 vertices, ~9000 triangles)
	// -------------------------------------------------------------------------
	{
		GameContext context;
		context.config.init(kConfigRoots);

		t_collision_mesh_id terminatorMeshId = context.collisionBodyManager.loadCollisionModel(
			"assets/Models/spaceships/terminator/spaceship_terminator.collision.obj"
		);
		const CollisionModel &collisionModel = context.collisionBodyManager.getCollisionModel(terminatorMeshId);

		CollisionMeshInstance meshInstance{
			Vector3Zeros,
			Vector3Zeros,
			Vector3Zeros,
			Vector3{1.0f, 1.0f, 1.0f},
			QuaternionIdentity()
		};

		std::vector<double> terminatorSamples;
		terminatorSamples.reserve(kIterations);
		for (int i = 0; i < kIterations; ++i)
		{
			Vector3 spherePrev{0.0f, 0.0f, 5.0f};
			Vector3 sphereVel{0.0f, 0.0f, -2.0f};
			CollisionInterval interval{0.0f, 1.0f};

			auto t0 = std::chrono::high_resolution_clock::now();
			auto hit = sweepSphereAgainstMesh(collisionModel, meshInstance, spherePrev, sphereVel, 1.0f, interval);
			auto t1 = std::chrono::high_resolution_clock::now();
			terminatorSamples.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
			(void)hit;
		}
		printStats("2. Ship vs Terminator (inside AABB)", computeStats(terminatorSamples));
	}

	// -------------------------------------------------------------------------
	// Scenario 3: Full DetectEntityCollision System Profile
	// -------------------------------------------------------------------------
	{
		GameContext context;
		context.config.init(kConfigRoots);

		// Spawn Player
		spaceship::factory::SpawnParams playerParams;
		playerParams.position = Vector3Zeros;
		playerParams.radius = 1.0f;
		playerParams.faction = faction::FAC_BLUE;
		spaceship::factory::spawnConfiguredSpaceship(context, "player", playerParams);

		// Spawn Asteroid right at the player (colliding/overlapping)
		spawnAsteroid(context, Vector3{0.0f, 0.0f, 80.0f}, Vector3{0.0f, 0.0f, -1.0f}, 100.0f);

		// Spawn 4 enemy ships close by
		const std::vector<std::string> ships = {"basic", "elite", "mothership", "terminator"};
		for (size_t i = 0; i < ships.size(); ++i)
		{
			spaceship::factory::SpawnParams p;
			p.position = Vector3{static_cast<float>(i * 10.0f - 15.0f), 0.0f, static_cast<float>(i * 8.0f + 10.0f)};
			p.radius = 2.0f;
			p.faction = faction::FAC_RED;
			spaceship::factory::spawnConfiguredSpaceship(context, ships[i], p);
		}

		// Breakdown timing for Scenario 3
		double timeGatherUs = 0.0;
		double timeBroadPhaseUs = 0.0;
		double timeNarrowPhaseUs = 0.0;
		size_t broadPhasePassCount = 0;
		size_t narrowPhaseHitCount = 0;
		size_t totalPairsChecked = 0;
		size_t totalEntities = 0;

		for (int frame = 0; frame < kIterations; ++frame)
		{
			auto tStart = std::chrono::high_resolution_clock::now();

			// Gather
			std::vector<EntityData> entities;
			for (auto [entity, position, body] : context.registry.view<Position, CollisionBody>().each()) {
				Vector3 velocity = position.value - position.prevValue;
				int faction = context.registry.any_of<tag::Bullet>(entity) << 0;
				const CollisionBodyModel *cbm = context.registry.try_get<CollisionBodyModel>(entity);
				const RenderBody *rb = context.registry.try_get<RenderBody>(entity);
				float effRad = body.radius;
				if (cbm != nullptr && rb != nullptr) {
					float pr = context.collisionBodyManager.getCollisionRadius(cbm->modelID, rb->translation, rb->scale, rb->rotation);
					effRad = std::max(effRad, pr);
				}
				const auto *asmComp = context.registry.try_get<collision::Assembly>(entity);
				entt::entity asmRoot = asmComp ? asmComp->root : entt::null;
				entities.emplace_back(EntityData{entity, position.value - velocity, velocity, effRad, faction, asmRoot, cbm, rb});
			}
			auto tGather = std::chrono::high_resolution_clock::now();
			timeGatherUs += std::chrono::duration<double, std::micro>(tGather - tStart).count();
			totalEntities = entities.size();

			// Pairs
			for (size_t i = 0; i < entities.size(); ++i) {
				const auto &A = entities[i];
				for (size_t j = i + 1; j < entities.size(); ++j) {
					const auto &B = entities[j];
					totalPairsChecked++;
					if ((A.faction & B.faction) != 0) continue;
					if (A.assemblyRoot != entt::null && A.assemblyRoot == B.assemblyRoot) continue;

					auto tBroadStart = std::chrono::high_resolution_clock::now();
					float combinedRad = A.rad + B.rad;
					auto interval = calculateCollisionInterval(A.pos, A.vel, B.pos, B.vel, combinedRad);
					bool willCol = willCollide(interval, 1.0f);
					auto tBroadEnd = std::chrono::high_resolution_clock::now();
					timeBroadPhaseUs += std::chrono::duration<double, std::micro>(tBroadEnd - tBroadStart).count();

					if (!willCol) continue;
					broadPhasePassCount++;

					auto tNarrowStart = std::chrono::high_resolution_clock::now();
					auto meshRes = processMeshCollision(context, A, B, *interval);
					auto tNarrowEnd = std::chrono::high_resolution_clock::now();
					timeNarrowPhaseUs += std::chrono::duration<double, std::micro>(tNarrowEnd - tNarrowStart).count();

					if (meshRes.usesMeshNarrowPhase && !meshRes.hit) continue;
					narrowPhaseHitCount++;
				}
			}
		}

		std::cout << "\n--- Scenario 3 Breakdown (averaged per frame) ---\n";
		std::cout << "Total Entities:        " << totalEntities << "\n";
		std::cout << "Pairs Checked:         " << (totalPairsChecked / kIterations) << "\n";
		std::cout << "Broadphase Overlaps:   " << (broadPhasePassCount / kIterations) << "\n";
		std::cout << "Narrowphase Hits:      " << (narrowPhaseHitCount / kIterations) << "\n";
		std::cout << "Gather Time:           " << (timeGatherUs / kIterations) << " us (" << (timeGatherUs / kIterations / 1000.0) << " ms)\n";
		std::cout << "Broadphase Time:       " << (timeBroadPhaseUs / kIterations) << " us (" << (timeBroadPhaseUs / kIterations / 1000.0) << " ms)\n";
		std::cout << "Narrowphase Time:      " << (timeNarrowPhaseUs / kIterations) << " us (" << (timeNarrowPhaseUs / kIterations / 1000.0) << " ms)\n";
		std::cout << "--------------------------------------------------\n\n";
	}

	std::cout << "========================================================================\n";
	CloseWindow();
	return 0;
}
