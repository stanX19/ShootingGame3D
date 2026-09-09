#include "includes.hpp"
#include "game_context.hpp"
#include "renderer.hpp"
#include "components/physics.hpp"
#include "components/render.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#if __has_include(<valgrind/valgrind.h>)
#include <valgrind/valgrind.h>
#else
#ifndef RUNNING_ON_VALGRIND
#define RUNNING_ON_VALGRIND 0
#endif
#endif

#if __has_include(<valgrind/memcheck.h>)
#include <valgrind/memcheck.h>
#endif

namespace {

void runScopedModelRender(Camera& camera)
{
    std::cout << "[Test Scope] Initializing GameContext and Renderer inside local scope...\n";
    GameContext context;
    Renderer renderer(camera, context);

    // 1. Load 3 models from assets
    std::cout << "[Test Scope] Loading 3 models from assets...\n";
    t_model_id m1 = context.modelManager.loadModel("assets/models/spaceships/player/spaceship_player.obj");
    t_model_id m2 = context.modelManager.loadModel("assets/models/asteroid/asteroid_small.obj");
    t_model_id m3 = context.modelManager.loadModel("assets/models/spaceships/Spaceship1.obj");

    // 2. Create 3 primitive / procedural models
    std::cout << "[Test Scope] Creating 3 primitive models...\n";
    t_model_id m4 = context.modelManager.createSphere(16, 16, 1.0f);
    t_model_id m5 = context.modelManager.createCube(2.0f, 2.0f, 2.0f);
    t_model_id m6 = context.modelManager.createCylinder(16, 1.0f, 2.0f);

    // 3. Emplace a light source entity (required by lighted shader)
    auto light = context.registry.create();
    context.registry.emplace<Position>(light, Vector3{100.0f, 100.0f, 100.0f});
    context.registry.emplace<RenderBody>(light, m4, WHITE, 1.0f);
    context.registry.emplace<tag::LightSource>(light);

    // 4. Emplace renderable entities for all 6 models
    t_model_id models[] = {m1, m2, m3, m4, m5, m6};
    for (int i = 0; i < 6; ++i)
    {
        auto entity = context.registry.create();
        context.registry.emplace<Position>(entity, Vector3{static_cast<float>(i * 3 - 7.5f), 0.0f, 0.0f});
        context.registry.emplace<RenderBody>(entity, models[i], WHITE, 1.0f);
        context.registry.emplace<tag::Shaded>(entity);
    }

    // 5. Render once with Renderer
    std::cout << "[Test Scope] Performing single frame render...\n";
    renderer.Render(0.016f);

    // 6. Unload models from manager
    std::cout << "[Test Scope] Calling ModelManager::unloadAll()...\n";
    context.modelManager.unloadAll();

    std::cout << "[Test Scope] Exiting local scope to invoke all destructors...\n";
}

} // namespace

int main(int argc, char* argv[])
{
    bool isChild = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--child") == 0)
        {
            isChild = true;
            break;
        }
    }

    // Check if Valgrind exists on this system
    if (!isChild && !RUNNING_ON_VALGRIND)
    {
        const int valgrindAvailable = std::system("command -v valgrind > /dev/null 2>&1");
        if (valgrindAvailable == 0)
        {
            std::cout << "[Valgrind Test] Valgrind detected on system. Re-executing test under Valgrind...\n";
            std::string cmd = "valgrind --leak-check=full --quiet \"" + std::string(argv[0]) + "\" --child";
            int ret = std::system(cmd.c_str());
            int exitCode = WEXITSTATUS(ret);
            if (exitCode != 0)
            {
                std::cerr << "[Valgrind Test] FAILED: Scoped leak test returned non-zero exit code: " << exitCode << "\n";
                return exitCode;
            }
            std::cout << "[Valgrind Test] PASSED: Clean execution under Valgrind with zero scoped memory leaks.\n";
            return 0;
        }
        else
        {
            std::cout << "[Valgrind Test] Valgrind is not installed on this system. Running native test...\n";
        }
    }

    // Raylib window initialization (hidden off-screen for test)
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(640, 480, "Model Memory Leak Test");

    Camera camera{};
    camera.position = Vector3{0.0f, 10.0f, 20.0f};
    camera.target = Vector3{0.0f, 0.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Execute scoped test
    runScopedModelRender(camera);

    // Mid-process Valgrind leak check
#if defined(VALGRIND_COUNT_LEAKS) && defined(VALGRIND_DO_QUICK_LEAK_CHECK)
    if (RUNNING_ON_VALGRIND)
    {
        unsigned long leaked = 0, dubious = 0, reachable = 0, suppressed = 0;
        VALGRIND_DO_QUICK_LEAK_CHECK;
        VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed);
        std::cout << "[Valgrind] Post-Scope Leak Stats: definite=" << leaked 
                  << " bytes, dubious=" << dubious 
                  << " bytes, reachable=" << reachable 
                  << " bytes, suppressed=" << suppressed << " bytes.\n";

        if (leaked > 0)
        {
            std::cerr << "[Valgrind] ERROR: " << leaked 
                      << " definitely leaked bytes remaining after scope destruction!\n";
            CloseWindow();
            return 1;
        }
        std::cout << "[Valgrind] Post-Scope Check: 0 definite memory leaks detected.\n";
    }
#endif

    CloseWindow();
    std::cout << "[Test] Completed successfully.\n";
    return 0;
}
