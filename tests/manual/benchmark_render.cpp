#include "shoot_3d.hpp"
#include "renderer.hpp"
#include "battlefield_hud_renderer.hpp"
#include "rlgl.h"
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cstdint>

namespace {
	enum RenderProbeId : size_t {
		PROBE_CLEAR_BACKGROUND = 0,
		PROBE_SKYBOX_PASS,
		PROBE_BEGIN_MODE_3D,
		PROBE_LIGHT_SOURCE,
		PROBE_UNSHADED_ENTITIES,
		PROBE_SHADED_ENTITIES,
		PROBE_BOUNDARY_WARNING,
		PROBE_ENERGY_SHIELD,
		PROBE_DEBUG_DRAW,
		PROBE_END_MODE_3D_FLUSH,
		PROBE_HUD_PASS,
		PROBE_TOTAL_RENDER,
		PROBE_COUNT
	};

	struct alignas(64) RenderMetric {
		const char *name = "";
		uint64_t totalNanos = 0;
		uint64_t minNanos = UINT64_MAX;
		uint64_t maxNanos = 0;
		uint32_t sampleCount = 0;
	};

	struct RenderFrameSample {
		size_t frameIndex = 0;
		double frameTimeMs = 0.0;
		size_t totalRenderBodies = 0;
		size_t unshadedBodies = 0;
		size_t shadedBodies = 0;
		size_t bulletCount = 0;
		size_t particleCount = 0;
	};

	enum class BenchmarkState {
		WARMUP,
		BENCHMARKING,
		REPORT
	};

	template <RenderProbeId ID, typename Func>
	inline void runRenderProbe(RenderMetric (&metrics)[PROBE_COUNT], bool profiling, Func &&func) {
		if (!profiling) {
			func();
			return;
		}

		const auto start = std::chrono::high_resolution_clock::now();
		func();
		const auto end = std::chrono::high_resolution_clock::now();
		const uint64_t duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

		RenderMetric &m = metrics[ID];
		m.totalNanos += duration;
		if (duration < m.minNanos) m.minNanos = duration;
		if (duration > m.maxNanos) m.maxNanos = duration;
		m.sampleCount++;
	}

	void initProbeNames(RenderMetric (&metrics)[PROBE_COUNT]) {
		metrics[PROBE_CLEAR_BACKGROUND].name = "ClearBackground";
		metrics[PROBE_SKYBOX_PASS].name = "drawEntitiesWithSkyboxShader";
		metrics[PROBE_BEGIN_MODE_3D].name = "BeginMode3D";
		metrics[PROBE_LIGHT_SOURCE].name = "handleLightSource";
		metrics[PROBE_UNSHADED_ENTITIES].name = "drawEntitiesWithoutShader (Particles/Bullets/Exp)";
		metrics[PROBE_SHADED_ENTITIES].name = "drawEntitiesWithShader (Ships/Asteroids)";
		metrics[PROBE_BOUNDARY_WARNING].name = "drawBoundaryWarning";
		metrics[PROBE_ENERGY_SHIELD].name = "drawEnergyShield";
		metrics[PROBE_DEBUG_DRAW].name = "drawDebug";
		metrics[PROBE_END_MODE_3D_FLUSH].name = "EndMode3D (Raylib/OpenGL Buffer Flush)";
		metrics[PROBE_HUD_PASS].name = "hudRenderer.RenderAll";
		metrics[PROBE_TOTAL_RENDER].name = "TOTAL RENDER FRAME TIME";
	}

	void resetProbes(RenderMetric (&metrics)[PROBE_COUNT]) {
		for (size_t i = 0; i < PROBE_COUNT; i++) {
			metrics[i].totalNanos = 0;
			metrics[i].minNanos = UINT64_MAX;
			metrics[i].maxNanos = 0;
			metrics[i].sampleCount = 0;
		}
	}
}

class BenchmarkRenderer {
public:
	static void renderInstrumented(Renderer &renderer, BattlefieldHUDRenderer &hudRenderer, float dt, RenderMetric (&metrics)[PROBE_COUNT], bool profiling) {
		renderer.currentDt = dt;

		const auto renderStart = std::chrono::high_resolution_clock::now();

		runRenderProbe<PROBE_CLEAR_BACKGROUND>(metrics, profiling, [&]() {
			ClearBackground(BLACK);
		});

		runRenderProbe<PROBE_SKYBOX_PASS>(metrics, profiling, [&]() {
			renderer.drawEntitiesWithSkyboxShader();
		});

		runRenderProbe<PROBE_BEGIN_MODE_3D>(metrics, profiling, [&]() {
			BeginMode3D(renderer.camera);
		});

		runRenderProbe<PROBE_LIGHT_SOURCE>(metrics, profiling, [&]() {
			renderer.handleLightSource();
		});

		runRenderProbe<PROBE_UNSHADED_ENTITIES>(metrics, profiling, [&]() {
			renderer.drawEntitiesWithoutShader();
		});

		runRenderProbe<PROBE_SHADED_ENTITIES>(metrics, profiling, [&]() {
			renderer.drawEntitiesWithShader();
		});

		runRenderProbe<PROBE_BOUNDARY_WARNING>(metrics, profiling, [&]() {
			renderer.drawBoundaryWarning();
		});

		runRenderProbe<PROBE_ENERGY_SHIELD>(metrics, profiling, [&]() {
			renderer.drawEnergyShield();
		});

		runRenderProbe<PROBE_DEBUG_DRAW>(metrics, profiling, [&]() {
			renderer.drawDebug();
		});

		runRenderProbe<PROBE_END_MODE_3D_FLUSH>(metrics, profiling, [&]() {
			EndMode3D();
		});

		runRenderProbe<PROBE_HUD_PASS>(metrics, profiling, [&]() {
			hudRenderer.RenderAll(dt);
		});

		const auto renderEnd = std::chrono::high_resolution_clock::now();
		if (profiling) {
			const uint64_t totalNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(renderEnd - renderStart).count();
			RenderMetric &m = metrics[PROBE_TOTAL_RENDER];
			m.totalNanos += totalNanos;
			if (totalNanos < m.minNanos) m.minNanos = totalNanos;
			if (totalNanos > m.maxNanos) m.maxNanos = totalNanos;
			m.sampleCount++;
		}
	}
};

int main() {
	const int screenWidth = 1280;
	const int screenHeight = 720;

	InitWindow(screenWidth, screenHeight, "Renderer Sub-Pass Benchmark");
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

	RenderMetric metrics[PROBE_COUNT];
	initProbeNames(metrics);

	std::vector<RenderFrameSample> samples;
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

		// Spectator camera orbit
		float camAngle = GetTime() * 0.06f;
		context.mainCamera.position = Vector3{
			std::sin(camAngle) * 1500.0f,
			700.0f,
			std::cos(camAngle) * 1500.0f
		};

		if (state == BenchmarkState::WARMUP) {
			if (GetTime() >= 60.0) {
				std::cout << ">>> BENCHMARK WARMUP REACHED 60s (FPS: " << smoothedFps << "). Exiting automatically without lag spike. <<<" << std::endl;
				break;
			}
			if ((smoothedFps < 30.0f && currentFrame > 120) || IsKeyPressed(KEY_B) || IsKeyPressed(KEY_SPACE) || GetTime() > 40.0) {
				state = BenchmarkState::BENCHMARKING;
				benchmarkTimer = 0.0;
				resetProbes(metrics);
				samples.clear();
				std::cout << "\n=================================================================================" << std::endl;
				std::cout << ">>> RENDER SUB-PASS BENCHMARK TRIGGERED (FPS: " << smoothedFps << ")! Profiling for 10s... <<<" << std::endl;
				std::cout << "=================================================================================\n" << std::endl;
			}
		} else if (state == BenchmarkState::BENCHMARKING) {
			benchmarkTimer += dt;
			if (benchmarkTimer >= BENCHMARK_DURATION) {
				state = BenchmarkState::REPORT;
			}
		}

		const bool isProfiling = (state == BenchmarkState::BENCHMARKING);

		// --- Run Simulation Systems ---
		ecs_systems::playerRespawn(context, dt);
		ecs_systems::aiFindTarget(context, dt);
		ecs_systems::aiMoveControl(context, dt);
		ecs_systems::aiShootControl(context, dt);
		ecs_systems::processMoveRequest(context, dt);
		ecs_systems::ammoReload(context, dt);
		ecs_systems::bulletTargetAim(context, dt);
		ecs_systems::weaponParentControlAim(context, dt);
		ecs_systems::weaponParentControlShoot(context, dt);
		ecs_systems::playerShootControl(context, dt);
		ecs_systems::weaponUpdateCooldown(context, dt);
		ecs_systems::weaponUpdateCanFire(context, dt);
		ecs_systems::weaponUpdateCharged(context, dt);
		ecs_systems::weaponShoot(context, dt);
		ecs_systems::weaponUpdateFireStatus(context, dt);
		ecs_systems::entityMovement(context, dt);
		ecs_systems::entityAnchor(context, dt);
		ecs_systems::entityTransformation(context, dt);
		ecs_systems::detectEntityCollision(context, dt);
		context.dispatcher.update();
		context.soundManager.update(context.mainCamera);
		ecs_systems::soundSfx(context, dt);
		ecs_systems::energyShield(context, dt);
		ecs_systems::syncModelRotation(context, dt);
		ecs_systems::cameraFollowPlayer(context, dt);

		// --- Render with Direct Sub-Pass Probing ---
		BeginDrawing();
		BenchmarkRenderer::renderInstrumented(renderer, hudRenderer, dt, metrics, isProfiling);

		if (isProfiling) {
			RenderFrameSample s;
			s.frameIndex = currentFrame;
			s.frameTimeMs = static_cast<double>(dt) * 1000.0;
			s.totalRenderBodies = context.registry.storage<RenderBody>().size();
			s.shadedBodies = context.registry.view<RenderBody, tag::Shaded>().size_hint();
			s.unshadedBodies = s.totalRenderBodies > s.shadedBodies ? (s.totalRenderBodies - s.shadedBodies) : 0;
			s.bulletCount = context.registry.storage<tag::Bullet>().size();
			s.particleCount = context.registry.view<RenderBody, Lifespan>().size_hint();
			samples.push_back(s);
		}

		// Simulation Cleanup Systems
		ecs_systems::blueUnitRespawn(context, dt);
		ecs_systems::redUnitRespawn(context, dt);
		ecs_systems::asteroidRespawn(context, dt);
		ecs_systems::entityAnchorRelease(context, dt);
		ecs_systems::entityLifetime(context, dt);
		ecs_systems::cleanOutOfBound(context, dt);
		ecs_systems::delayedDamage(context, dt);
		ecs_systems::hpCleanup(context, dt);
		ecs_systems::hpRegen(context, dt);
		ecs_systems::spawnTrailParticles(context, dt);

		// Realtime UI Overlay
		if (state == BenchmarkState::WARMUP) {
			DrawRectangle(10, 10, 420, 95, ColorAlpha(BLACK, 0.75f));
			DrawText("RENDER BENCHMARK: Warmup...", 20, 20, 18, YELLOW);
			DrawText(TextFormat("FPS: %.1f (Drop < 30 triggers profiling)", smoothedFps), 20, 45, 16, smoothedFps < 30 ? RED : GREEN);
			DrawText("Press 'B' or SPACE to force trigger benchmark", 20, 72, 13, GRAY);
		} else if (state == BenchmarkState::BENCHMARKING) {
			DrawRectangle(10, 10, 420, 115, ColorAlpha(BLACK, 0.85f));
			DrawText(TextFormat("RENDER PROFILING: %.1fs / %.0fs", benchmarkTimer, BENCHMARK_DURATION), 20, 20, 18, RED);
			DrawText(TextFormat("FPS: %.1f | Frame: %.2f ms", smoothedFps, dt * 1000.0f), 20, 45, 16, WHITE);
			DrawText(TextFormat("RenderBodies: %zu (Unshaded: %zu, Shaded: %zu)",
				context.registry.storage<RenderBody>().size(),
				context.registry.storage<RenderBody>().size() - context.registry.view<RenderBody, tag::Shaded>().size_hint(),
				context.registry.view<RenderBody, tag::Shaded>().size_hint()), 20, 68, 14, SKYBLUE);
			DrawText(TextFormat("Bullets: %zu | Particles/Lifespans: %zu",
				context.registry.storage<tag::Bullet>().size(),
				context.registry.view<RenderBody, Lifespan>().size_hint()), 20, 90, 13, LIGHTGRAY);
		} else {
			DrawRectangle(10, 10, 480, 85, ColorAlpha(BLACK, 0.85f));
			DrawText("RENDER BENCHMARK COMPLETE!", 20, 20, 20, GREEN);
			DrawText("Sub-pass breakdown printed to console!", 20, 45, 16, WHITE);
			DrawText("Press 'R' to rerun, ESC to exit.", 20, 68, 13, GRAY);
		}

		EndDrawing();

		if (state == BenchmarkState::REPORT && !samples.empty()) {
			std::cout << "\n=================================================================================" << std::endl;
			std::cout << "                      RENDER SUB-PASS BENCHMARK REPORT                           " << std::endl;
			std::cout << "=================================================================================" << std::endl;
			std::cout << "Total Frames Sampled: " << samples.size() << std::endl;
			std::cout << "Benchmark Duration:   " << std::fixed << std::setprecision(2) << benchmarkTimer << " s" << std::endl;
			const double avgFps = samples.size() / benchmarkTimer;
			std::cout << "Average FPS:          " << std::fixed << std::setprecision(1) << avgFps << std::endl;

			const double totalRenderMs = static_cast<double>(metrics[PROBE_TOTAL_RENDER].totalNanos) / 1.0e6;
			const double avgRenderMs = (totalRenderMs / samples.size());
			std::cout << "Total Render Time:    " << std::fixed << std::setprecision(2) << totalRenderMs
					  << " ms (Avg: " << avgRenderMs << " ms/frame)" << std::endl;

			std::cout << "\n--- RENDER SUB-PASS BREAKDOWN ---" << std::endl;
			std::cout << std::left << std::setw(50) << "Render Sub-Pass"
					  << std::right << std::setw(12) << "Total (ms)"
					  << std::setw(12) << "Avg (ms)"
					  << std::setw(12) << "Max (ms)"
					  << std::setw(10) << "% Render" << std::endl;
			std::cout << std::string(96, '-') << std::endl;

			for (size_t i = 0; i < PROBE_COUNT - 1; i++) {
				const auto &m = metrics[i];
				if (m.sampleCount == 0 && metrics[PROBE_TOTAL_RENDER].totalNanos == 0) continue;
				const double subMs = static_cast<double>(m.totalNanos) / 1.0e6;
				const double avgSubMs = m.sampleCount > 0 ? (static_cast<double>(m.totalNanos) / m.sampleCount) / 1.0e6 : 0.0;
				const double maxSubMs = static_cast<double>(m.maxNanos) / 1.0e6;
				const double pct = metrics[PROBE_TOTAL_RENDER].totalNanos > 0
					? (static_cast<double>(m.totalNanos) / metrics[PROBE_TOTAL_RENDER].totalNanos * 100.0) : 0.0;

				std::cout << std::left << std::setw(50) << m.name
						  << std::right << std::setw(12) << std::fixed << std::setprecision(2) << subMs
						  << std::setw(12) << std::setprecision(3) << avgSubMs
						  << std::setw(12) << std::setprecision(3) << maxSubMs
						  << std::setw(9) << std::setprecision(1) << pct << "%" << std::endl;
			}

			std::cout << "\n--- POPULATION METRICS DURING BENCHMARK ---" << std::endl;
			size_t avgRen = 0, maxRen = 0;
			size_t avgUnsh = 0, maxUnsh = 0;
			size_t avgShad = 0, maxShad = 0;
			size_t avgBull = 0, maxBull = 0;
			size_t avgPart = 0, maxPart = 0;
			for (const auto &s : samples) {
				avgRen += s.totalRenderBodies; maxRen = std::max(maxRen, s.totalRenderBodies);
				avgUnsh += s.unshadedBodies; maxUnsh = std::max(maxUnsh, s.unshadedBodies);
				avgShad += s.shadedBodies; maxShad = std::max(maxShad, s.shadedBodies);
				avgBull += s.bulletCount; maxBull = std::max(maxBull, s.bulletCount);
				avgPart += s.particleCount; maxPart = std::max(maxPart, s.particleCount);
			}
			avgRen /= samples.size();
			avgUnsh /= samples.size();
			avgShad /= samples.size();
			avgBull /= samples.size();
			avgPart /= samples.size();

			std::cout << "Total RenderBodies:    Avg = " << std::setw(5) << avgRen << " | Max = " << maxRen << std::endl;
			std::cout << "Unshaded Entities:     Avg = " << std::setw(5) << avgUnsh << " | Max = " << maxUnsh << " (Particles, Bullets, Explosions)" << std::endl;
			std::cout << "Shaded Entities:       Avg = " << std::setw(5) << avgShad << " | Max = " << maxShad << " (Ships, Asteroids)" << std::endl;
			std::cout << "Bullets:               Avg = " << std::setw(5) << avgBull << " | Max = " << maxBull << std::endl;
			std::cout << "Lifespan/Particles:    Avg = " << std::setw(5) << avgPart << " | Max = " << maxPart << std::endl;
			std::cout << "=================================================================================\n" << std::endl;

			samples.clear();
			break; // Auto-close after report
		}
	}

	context.soundManager.shutdown();
	context.modelManager.unloadAll();
	CloseWindow();
	return 0;
}
