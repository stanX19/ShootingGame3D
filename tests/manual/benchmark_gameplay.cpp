#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "battlefield_hud_renderer.hpp"
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <cstdint>

namespace {
	enum SystemId : size_t {
		SYS_PLAYER_RESPAWN = 0,
		SYS_AI_FIND_TARGET,
		SYS_AI_MOVE_CONTROL,
		SYS_AI_SHOOT_CONTROL,
		SYS_PROCESS_MOVE_REQUEST,

		SYS_AMMO_RELOAD,
		SYS_BULLET_TARGET_AIM,
		SYS_WEAPON_PARENT_CONTROL_AIM,
		SYS_WEAPON_PARENT_CONTROL_SHOOT,
		SYS_PLAYER_SHOOT_CONTROL,
		SYS_WEAPON_UPDATE_COOLDOWN,
		SYS_WEAPON_UPDATE_CAN_FIRE,
		SYS_WEAPON_UPDATE_CHARGED,
		SYS_WEAPON_SHOOT,
		SYS_WEAPON_UPDATE_FIRE_STATUS,

		SYS_ENTITY_MOVEMENT,
		SYS_ENTITY_ANCHOR,
		SYS_ENTITY_TRANSFORMATION,

		SYS_DETECT_COLLISION,
		SYS_DISPATCHER_UPDATE,
		SYS_SOUND_UPDATE,
		SYS_SOUND_SFX,

		SYS_ENERGY_SHIELD,
		SYS_SYNC_MODEL_ROTATION,
		SYS_CAMERA_FOLLOW_PLAYER,

		SYS_RENDERER_RENDER,
		SYS_HUD_RENDERER_RENDER,

		SYS_BLUE_UNIT_RESPAWN,
		SYS_RED_UNIT_RESPAWN,
		SYS_ASTEROID_RESPAWN,
		SYS_ENTITY_ANCHOR_RELEASE,
		SYS_ENTITY_LIFETIME,
		SYS_CLEAN_OUT_OF_BOUND,
		SYS_DELAYED_DAMAGE,
		SYS_HP_CLEANUP,
		SYS_HP_REGEN,
		SYS_SPAWN_TRAIL_PARTICLES,

		SYS_COUNT
	};

	struct alignas(64) SystemMetric {
		const char *name = "";
		uint64_t totalNanos = 0;
		uint64_t minNanos = UINT64_MAX;
		uint64_t maxNanos = 0;
		uint32_t sampleCount = 0;
	};

	struct BenchmarkSample {
		size_t frameIndex = 0;
		double frameTimeMs = 0.0;
		size_t totalEntities = 0;
		size_t bulletCount = 0;
		size_t collisionBodyCount = 0;
		size_t renderBodyCount = 0;
		size_t lifespanCount = 0;
		size_t trailCount = 0;
	};

	enum class BenchmarkState {
		WARMUP,
		BENCHMARKING,
		REPORT
	};

	template <SystemId ID, typename Func>
	inline void runProfiled(SystemMetric (&metrics)[SYS_COUNT], bool profiling, Func &&func) {
		if (!profiling) {
			func();
			return;
		}

		const auto start = std::chrono::high_resolution_clock::now();
		func();
		const auto end = std::chrono::high_resolution_clock::now();
		const uint64_t duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

		SystemMetric &m = metrics[ID];
		m.totalNanos += duration;
		if (duration < m.minNanos) m.minNanos = duration;
		if (duration > m.maxNanos) m.maxNanos = duration;
		m.sampleCount++;
	}

	void initMetricNames(SystemMetric (&metrics)[SYS_COUNT]) {
		metrics[SYS_PLAYER_RESPAWN].name = "playerRespawn";
		metrics[SYS_AI_FIND_TARGET].name = "aiFindTarget";
		metrics[SYS_AI_MOVE_CONTROL].name = "aiMoveControl";
		metrics[SYS_AI_SHOOT_CONTROL].name = "aiShootControl";
		metrics[SYS_PROCESS_MOVE_REQUEST].name = "processMoveRequest";

		metrics[SYS_AMMO_RELOAD].name = "ammoReload";
		metrics[SYS_BULLET_TARGET_AIM].name = "bulletTargetAim";
		metrics[SYS_WEAPON_PARENT_CONTROL_AIM].name = "weaponParentControlAim";
		metrics[SYS_WEAPON_PARENT_CONTROL_SHOOT].name = "weaponParentControlShoot";
		metrics[SYS_PLAYER_SHOOT_CONTROL].name = "playerShootControl";
		metrics[SYS_WEAPON_UPDATE_COOLDOWN].name = "weaponUpdateCooldown";
		metrics[SYS_WEAPON_UPDATE_CAN_FIRE].name = "weaponUpdateCanFire";
		metrics[SYS_WEAPON_UPDATE_CHARGED].name = "weaponUpdateCharged";
		metrics[SYS_WEAPON_SHOOT].name = "weaponShoot";
		metrics[SYS_WEAPON_UPDATE_FIRE_STATUS].name = "weaponUpdateFireStatus";

		metrics[SYS_ENTITY_MOVEMENT].name = "entityMovement";
		metrics[SYS_ENTITY_ANCHOR].name = "entityAnchor";
		metrics[SYS_ENTITY_TRANSFORMATION].name = "entityTransformation";

		metrics[SYS_DETECT_COLLISION].name = "detectEntityCollision";
		metrics[SYS_DISPATCHER_UPDATE].name = "dispatcher.update";
		metrics[SYS_SOUND_UPDATE].name = "soundManager.update";
		metrics[SYS_SOUND_SFX].name = "soundSfx";

		metrics[SYS_ENERGY_SHIELD].name = "energyShield";
		metrics[SYS_SYNC_MODEL_ROTATION].name = "syncModelRotation";
		metrics[SYS_CAMERA_FOLLOW_PLAYER].name = "cameraFollowPlayer";

		metrics[SYS_RENDERER_RENDER].name = "renderer.Render";
		metrics[SYS_HUD_RENDERER_RENDER].name = "hudRenderer.RenderAll";

		metrics[SYS_BLUE_UNIT_RESPAWN].name = "blueUnitRespawn";
		metrics[SYS_RED_UNIT_RESPAWN].name = "redUnitRespawn";
		metrics[SYS_ASTEROID_RESPAWN].name = "asteroidRespawn";
		metrics[SYS_ENTITY_ANCHOR_RELEASE].name = "entityAnchorRelease";
		metrics[SYS_ENTITY_LIFETIME].name = "entityLifetime";
		metrics[SYS_CLEAN_OUT_OF_BOUND].name = "cleanOutOfBound";
		metrics[SYS_DELAYED_DAMAGE].name = "delayedDamage";
		metrics[SYS_HP_CLEANUP].name = "hpCleanup";
		metrics[SYS_HP_REGEN].name = "hpRegen";
		metrics[SYS_SPAWN_TRAIL_PARTICLES].name = "spawnTrailParticles";
	}

	void resetMetrics(SystemMetric (&metrics)[SYS_COUNT]) {
		for (size_t i = 0; i < SYS_COUNT; i++) {
			metrics[i].totalNanos = 0;
			metrics[i].minNanos = UINT64_MAX;
			metrics[i].maxNanos = 0;
			metrics[i].sampleCount = 0;
		}
	}
}

int main() {
	const int screenWidth = 1280;
	const int screenHeight = 720;

	InitWindow(screenWidth, screenHeight, "Gameplay Performance Benchmark");
	SetTargetFPS(60);

	GameContext context;
	context.config.init({
		{"audio", "assets/config/audio.json"},
		{"debug", "assets/config/debug.json"},
		{"game", "assets/config/game.json"},
		{"loadout", "assets/config/loadout.json"},
		{"physics", "assets/config/physics.json"},
		{"settings", "assets/config/settings.json"},
		{"sounds", "assets/config/sounds.json"},
		{"units", "assets/config/units.json"},
		{"weapons", "assets/config/weapons.json"},
		{"spaceship", "assets/config/spaceships.json"}
	});

	context.weaponRegistry.init(context.config);
	context.soundManager.init(context.config);
	weapon::utils::setUpRegistry(context);
	event::utils::hookAllListeners(context);
	spawnSunAndStars(context);

	context.mainCamera.position = Vector3{ 0.0f, 600.0f, 1200.0f };
	context.mainCamera.target = Vector3{ 0.0f, 0.0f, 0.0f };
	context.mainCamera.up = Vector3{ 0.0f, 1.0f, 0.0f };
	context.mainCamera.fovy = 60.0f;
	context.mainCamera.projection = CAMERA_PERSPECTIVE;

	Renderer renderer(context.mainCamera, context);
	BattlefieldHUDRenderer hudRenderer(context.mainCamera, context);

	SystemMetric metrics[SYS_COUNT];
	initMetricNames(metrics);

	std::vector<BenchmarkSample> samples;
	samples.reserve(10000);

	BenchmarkState state = BenchmarkState::WARMUP;
	double benchmarkTimer = 0.0;
	const double BENCHMARK_DURATION = 10.0;
	float smoothedFps = 60.0f;
	size_t currentFrame = 0;

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();
		currentFrame++;
		float instantaneousFps = dt > 0.0001f ? (1.0f / dt) : 60.0f;
		smoothedFps = smoothedFps * 0.92f + instantaneousFps * 0.08f;

		// Spectator camera slow orbital movement
		float camAngle = GetTime() * 0.06f;
		context.mainCamera.position = Vector3{
			std::sin(camAngle) * 1500.0f,
			700.0f,
			std::cos(camAngle) * 1500.0f
		};

		// State transitions
		if (state == BenchmarkState::WARMUP) {
			if (GetTime() >= 60.0) {
				std::cout << ">>> BENCHMARK WARMUP REACHED 60s (FPS: " << smoothedFps << "). Exiting automatically without lag spike. <<<" << std::endl;
				break;
			}
			if ((smoothedFps < 30.0f && currentFrame > 120) || IsKeyPressed(KEY_B) || IsKeyPressed(KEY_SPACE) || GetTime() > 40.0) {
				state = BenchmarkState::BENCHMARKING;
				benchmarkTimer = 0.0;
				resetMetrics(metrics);
				samples.clear();
				std::cout << "\n=================================================================================" << std::endl;
				std::cout << ">>> BENCHMARK TRIGGERED (FPS: " << smoothedFps << ")! Profiling for 10 seconds... <<<" << std::endl;
				std::cout << "=================================================================================\n" << std::endl;
			}
		} else if (state == BenchmarkState::BENCHMARKING) {
			benchmarkTimer += dt;
			if (benchmarkTimer >= BENCHMARK_DURATION) {
				state = BenchmarkState::REPORT;
			}
		}

		const bool isProfiling = (state == BenchmarkState::BENCHMARKING);

		// --- Frame Simulation Probes (O(1) inlined enum index, zero allocation) ---
		runProfiled<SYS_PLAYER_RESPAWN>(metrics, isProfiling, [&]() { ecs_systems::playerRespawn(context, dt); });
		runProfiled<SYS_AI_FIND_TARGET>(metrics, isProfiling, [&]() { ecs_systems::aiFindTarget(context, dt); });
		runProfiled<SYS_AI_MOVE_CONTROL>(metrics, isProfiling, [&]() { ecs_systems::aiMoveControl(context, dt); });
		runProfiled<SYS_AI_SHOOT_CONTROL>(metrics, isProfiling, [&]() { ecs_systems::aiShootControl(context, dt); });
		runProfiled<SYS_PROCESS_MOVE_REQUEST>(metrics, isProfiling, [&]() { ecs_systems::processMoveRequest(context, dt); });

		runProfiled<SYS_AMMO_RELOAD>(metrics, isProfiling, [&]() { ecs_systems::ammoReload(context, dt); });
		runProfiled<SYS_BULLET_TARGET_AIM>(metrics, isProfiling, [&]() { ecs_systems::bulletTargetAim(context, dt); });
		runProfiled<SYS_WEAPON_PARENT_CONTROL_AIM>(metrics, isProfiling, [&]() { ecs_systems::weaponParentControlAim(context, dt); });
		runProfiled<SYS_WEAPON_PARENT_CONTROL_SHOOT>(metrics, isProfiling, [&]() { ecs_systems::weaponParentControlShoot(context, dt); });
		runProfiled<SYS_PLAYER_SHOOT_CONTROL>(metrics, isProfiling, [&]() { ecs_systems::playerShootControl(context, dt); });
		runProfiled<SYS_WEAPON_UPDATE_COOLDOWN>(metrics, isProfiling, [&]() { ecs_systems::weaponUpdateCooldown(context, dt); });
		runProfiled<SYS_WEAPON_UPDATE_CAN_FIRE>(metrics, isProfiling, [&]() { ecs_systems::weaponUpdateCanFire(context, dt); });
		runProfiled<SYS_WEAPON_UPDATE_CHARGED>(metrics, isProfiling, [&]() { ecs_systems::weaponUpdateCharged(context, dt); });
		runProfiled<SYS_WEAPON_SHOOT>(metrics, isProfiling, [&]() { ecs_systems::weaponShoot(context, dt); });
		runProfiled<SYS_WEAPON_UPDATE_FIRE_STATUS>(metrics, isProfiling, [&]() { ecs_systems::weaponUpdateFireStatus(context, dt); });

		runProfiled<SYS_ENTITY_MOVEMENT>(metrics, isProfiling, [&]() { ecs_systems::entityMovement(context, dt); });
		runProfiled<SYS_ENTITY_ANCHOR>(metrics, isProfiling, [&]() { ecs_systems::entityAnchor(context, dt); });
		runProfiled<SYS_ENTITY_TRANSFORMATION>(metrics, isProfiling, [&]() { ecs_systems::entityTransformation(context, dt); });

		runProfiled<SYS_DETECT_COLLISION>(metrics, isProfiling, [&]() { ecs_systems::detectEntityCollision(context, dt); });
		runProfiled<SYS_DISPATCHER_UPDATE>(metrics, isProfiling, [&]() { context.dispatcher.update(); });
		runProfiled<SYS_SOUND_UPDATE>(metrics, isProfiling, [&]() { context.soundManager.update(context.mainCamera); });
		runProfiled<SYS_SOUND_SFX>(metrics, isProfiling, [&]() { ecs_systems::soundSfx(context, dt); });

		runProfiled<SYS_ENERGY_SHIELD>(metrics, isProfiling, [&]() { ecs_systems::energyShield(context, dt); });
		runProfiled<SYS_SYNC_MODEL_ROTATION>(metrics, isProfiling, [&]() { ecs_systems::syncModelRotation(context, dt); });
		runProfiled<SYS_CAMERA_FOLLOW_PLAYER>(metrics, isProfiling, [&]() { ecs_systems::cameraFollowPlayer(context, dt); });

		BeginDrawing();
		runProfiled<SYS_RENDERER_RENDER>(metrics, isProfiling, [&]() { renderer.Render(dt); });
		runProfiled<SYS_HUD_RENDERER_RENDER>(metrics, isProfiling, [&]() { hudRenderer.RenderAll(dt); });

		runProfiled<SYS_BLUE_UNIT_RESPAWN>(metrics, isProfiling, [&]() { ecs_systems::blueUnitRespawn(context, dt); });
		runProfiled<SYS_RED_UNIT_RESPAWN>(metrics, isProfiling, [&]() { ecs_systems::redUnitRespawn(context, dt); });
		runProfiled<SYS_ASTEROID_RESPAWN>(metrics, isProfiling, [&]() { ecs_systems::asteroidRespawn(context, dt); });
		runProfiled<SYS_ENTITY_ANCHOR_RELEASE>(metrics, isProfiling, [&]() { ecs_systems::entityAnchorRelease(context, dt); });
		runProfiled<SYS_ENTITY_LIFETIME>(metrics, isProfiling, [&]() { ecs_systems::entityLifetime(context, dt); });
		runProfiled<SYS_CLEAN_OUT_OF_BOUND>(metrics, isProfiling, [&]() { ecs_systems::cleanOutOfBound(context, dt); });
		runProfiled<SYS_DELAYED_DAMAGE>(metrics, isProfiling, [&]() { ecs_systems::delayedDamage(context, dt); });
		runProfiled<SYS_HP_CLEANUP>(metrics, isProfiling, [&]() { ecs_systems::hpCleanup(context, dt); });
		runProfiled<SYS_HP_REGEN>(metrics, isProfiling, [&]() { ecs_systems::hpRegen(context, dt); });
		runProfiled<SYS_SPAWN_TRAIL_PARTICLES>(metrics, isProfiling, [&]() { ecs_systems::spawnTrailParticles(context, dt); });

		// Entity metric capture during profiling
		if (isProfiling) {
			BenchmarkSample s;
			s.frameIndex = currentFrame;
			s.frameTimeMs = static_cast<double>(dt) * 1000.0;
			s.totalEntities = context.registry.storage<entt::entity>().size();
			s.bulletCount = context.registry.storage<tag::Bullet>().size();
			s.collisionBodyCount = context.registry.storage<CollisionBody>().size();
			s.renderBodyCount = context.registry.storage<RenderBody>().size();
			s.lifespanCount = context.registry.storage<Lifespan>().size();
			s.trailCount = context.registry.storage<SpawnsTrailParticles>().size();
			samples.push_back(s);
		}

		// Realtime UI Overlay
		if (state == BenchmarkState::WARMUP) {
			DrawRectangle(10, 10, 380, 95, ColorAlpha(BLACK, 0.75f));
			DrawText("WARMUP: Fleets Battling...", 20, 20, 18, YELLOW);
			DrawText(TextFormat("FPS: %.1f (Drop < 30 triggers 10s benchmark)", smoothedFps), 20, 45, 16, smoothedFps < 30 ? RED : GREEN);
			DrawText("Press 'B' or SPACE to force trigger benchmark", 20, 72, 13, GRAY);
		} else if (state == BenchmarkState::BENCHMARKING) {
			size_t entCount = context.registry.storage<entt::entity>().size();
			size_t bulCount = context.registry.storage<tag::Bullet>().size();
			size_t colCount = context.registry.storage<CollisionBody>().size();
			size_t renCount = context.registry.storage<RenderBody>().size();

			DrawRectangle(10, 10, 380, 110, ColorAlpha(BLACK, 0.85f));
			DrawText(TextFormat("BENCHMARKING: %.1fs / %.0fs", benchmarkTimer, BENCHMARK_DURATION), 20, 20, 18, RED);
			DrawText(TextFormat("FPS: %.1f | Frame: %.2f ms", smoothedFps, dt * 1000.0f), 20, 45, 16, WHITE);
			DrawText(TextFormat("Entities: %zu | Bullets: %zu", entCount, bulCount), 20, 68, 14, SKYBLUE);
			DrawText(TextFormat("CollisionBodies: %zu | RenderBodies: %zu", colCount, renCount), 20, 88, 13, LIGHTGRAY);
		} else {
			DrawRectangle(10, 10, 480, 85, ColorAlpha(BLACK, 0.85f));
			DrawText("BENCHMARK COMPLETE!", 20, 20, 20, GREEN);
			DrawText("Diagnostic report printed to console!", 20, 45, 16, WHITE);
			DrawText("Press 'R' to rerun benchmark, ESC to exit.", 20, 68, 13, GRAY);
		}

		EndDrawing();

		// Post-benchmark diagnostic report (executed once)
		if (state == BenchmarkState::REPORT && !samples.empty()) {
			std::cout << "\n=================================================================================" << std::endl;
			std::cout << "                           GAMEPLAY BENCHMARK REPORT                             " << std::endl;
			std::cout << "=================================================================================" << std::endl;
			std::cout << "Total Frames Sampled: " << samples.size() << std::endl;
			std::cout << "Total Benchmark Time: " << std::fixed << std::setprecision(2) << benchmarkTimer << " s" << std::endl;
			const double avgFps = samples.size() / benchmarkTimer;
			std::cout << "Average FPS:          " << std::fixed << std::setprecision(1) << avgFps << std::endl;

			// Aggregate & sort metrics
			std::vector<SystemMetric> sortedMetrics;
			sortedMetrics.reserve(SYS_COUNT);
			uint64_t totalAllNanos = 0;
			for (size_t i = 0; i < SYS_COUNT; i++) {
				sortedMetrics.push_back(metrics[i]);
				totalAllNanos += metrics[i].totalNanos;
			}

			std::sort(sortedMetrics.begin(), sortedMetrics.end(), [](const SystemMetric &a, const SystemMetric &b) {
				return a.totalNanos > b.totalNanos;
			});

			std::cout << "\n--- RANKED SYSTEM PROCESSING FOOTPRINT ---" << std::endl;
			std::cout << std::left << std::setw(32) << "System / Component"
					  << std::right << std::setw(12) << "Total (ms)"
					  << std::setw(12) << "Avg (ms)"
					  << std::setw(12) << "Max (ms)"
					  << std::setw(10) << "% CPU" << std::endl;
			std::cout << std::string(78, '-') << std::endl;

			for (const auto &m : sortedMetrics) {
				if (m.sampleCount == 0) continue;
				const double totalMs = static_cast<double>(m.totalNanos) / 1.0e6;
				const double avgMs = (static_cast<double>(m.totalNanos) / m.sampleCount) / 1.0e6;
				const double maxMs = static_cast<double>(m.maxNanos) / 1.0e6;
				const double pct = totalAllNanos > 0 ? (static_cast<double>(m.totalNanos) / totalAllNanos * 100.0) : 0.0;

				std::cout << std::left << std::setw(32) << m.name
						  << std::right << std::setw(12) << std::fixed << std::setprecision(2) << totalMs
						  << std::setw(12) << std::setprecision(3) << avgMs
						  << std::setw(12) << std::setprecision(3) << maxMs
						  << std::setw(9) << std::setprecision(1) << pct << "%" << std::endl;
			}

			std::cout << "\n--- ENTITY & COMPONENT POPULATION METRICS ---" << std::endl;
			size_t maxEnt = 0, avgEnt = 0;
			size_t maxBul = 0, avgBul = 0;
			size_t maxCol = 0, avgCol = 0;
			size_t maxRen = 0, avgRen = 0;
			for (const auto &s : samples) {
				maxEnt = std::max(maxEnt, s.totalEntities);
				avgEnt += s.totalEntities;
				maxBul = std::max(maxBul, s.bulletCount);
				avgBul += s.bulletCount;
				maxCol = std::max(maxCol, s.collisionBodyCount);
				avgCol += s.collisionBodyCount;
				maxRen = std::max(maxRen, s.renderBodyCount);
				avgRen += s.renderBodyCount;
			}
			avgEnt /= samples.size();
			avgBul /= samples.size();
			avgCol /= samples.size();
			avgRen /= samples.size();

			std::cout << "Total Entities:   Avg = " << std::setw(5) << avgEnt << " | Max = " << maxEnt << std::endl;
			std::cout << "tag::Bullet:      Avg = " << std::setw(5) << avgBul << " | Max = " << maxBul << std::endl;
			std::cout << "CollisionBody:    Avg = " << std::setw(5) << avgCol << " | Max = " << maxCol << std::endl;
			std::cout << "RenderBody:       Avg = " << std::setw(5) << avgRen << " | Max = " << maxRen << std::endl;
			std::cout << "=================================================================================\n" << std::endl;

			samples.clear();
			break; // Auto-close after benchmarking report
		}
	}

	context.soundManager.shutdown();
	context.modelManager.unloadAll();
	CloseWindow();
	return 0;
}
