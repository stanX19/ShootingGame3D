# Project Context

This is the canonical tracked context for the 3D Space Shooter. Current source and the Makefile are authoritative; historical summaries and guides are reference material that must be verified.

## Objective and Stack

The project is a C++ 3D space-shooter game using raylib for graphics/audio and EnTT for entity-component-system (ECS) state. It includes player and AI movement, weapons, collisions, physics effects, rendering, audio, menus, settings, hangar loadouts, asteroids, and effects.

The current build uses C++20, GCC-compatible tooling, `-Wall -Wextra -Werror`, precompiled headers, and a vendored raylib/EnTT/json dependency set. The project is run in WSL Ubuntu.

## Repository and Tools

The Git repository root is `project/`. The workspace-level `AGENTS.md` is only a pointer to this file's companion rules.

Preferred host-side tasks use native `cmd /c`; PowerShell is an allowed fallback when it materially fits the task better. File changes use patch-based editing. Building, running code, testing, and Git use WSL.

From WSL, run commands from:

```text
/mnt/c/Users/user/Desktop/folders/cpp/shooting_game_3d/project
```

The current WSL toolchain inspected for this context is Ubuntu on WSL 2, GCC 13.3, GNU Make 4.3, and Git 2.43. CMake is not currently installed.

## Current Layout

```text
project/
├── main/                 Thin application entry point
├── headers/              Core declarations, ECS components, classes, utilities
├── srcs/classes/         Engine, game, renderer, HUD, audio, config, and UI
├── srcs/components/      Component/domain setup, mainly weapons and camera
├── srcs/entities/        Entity composition and spawn functions
├── srcs/events/          EnTT event handlers and listener registration
├── srcs/systems/         Ordered ECS simulation and control systems
├── srcs/entt_utils/      Entity/registry helpers
├── srcs/game_utils/      Game-specific helpers
├── srcs/utils/            General utilities
├── tests/                Current standalone test executables
├── assets/               Runtime config, models, sounds, snapshots
├── shaders/              GLSL shaders
├── includes/             Vendored dependencies
└── docs/                 Canonical engineering documentation
```

## Ownership and Runtime Flow

`main/main.cpp` constructs `Engine` and calls `run()`.

`Engine` initializes the window and shared `GameContext`, loads configuration, weapon metadata, and sound resources, then transitions among `MENU`, `GAME`, `HANGAR`, `SETTINGS`, and `EXIT` states.

`GameContext` owns shared configuration, model and sound managers, weapon registry, template and runtime EnTT registries, event dispatcher, current player, camera, and faction score data.

`Game::reset()` clears the runtime registry, hooks weapon and event listeners, spawns the player and scene entities, resets the camera, and starts game audio.

`Game::run()` owns the gameplay frame loop. The current order is broadly:

1. Player and AI input/target control.
2. Movement-request processing.
3. Weapon reload, aim, control, cooldown, charge, firing, and fire-status systems.
4. Entity movement, anchoring, and transformation.
5. Collision detection, event dispatch, sound updates, and shield handling.
6. Model rotation and camera following.
7. Rendering and HUD drawing.
8. Respawn, anchor release, lifetime, bounds, delayed damage, HP cleanup/regen, and trail-particle systems.
9. Gameplay input and state transitions.

The order is behaviorally significant. Do not move or early-return from frame-loop work without checking which timers, cleanup, particles, camera, audio, and state updates must still run.

## ECS Conventions

- Entities are lightweight EnTT IDs.
- Components are primarily data.
- Systems in `srcs/systems/` process explicit component views and receive `(GameContext &, float dt)`.
- Entity factories in `srcs/entities/` compose runtime entities.
- Events are declared in `headers/events.hpp`, handled in `srcs/events/`, and connected by the listener hook utility.
- Weapon construction is declared in `headers/weapons.hpp`, implemented under `srcs/components/weapon/`, and indexed by `weapon::WeaponRegistry`.
- Definitions and configuration should own canonical metadata; systems should own behavior.
- Anchored entities use parent/relative transform components and cleanup systems.

## Configuration and Data

`GameConfig` loads `assets/config/game_config.json`, exposes typed dotted-path getters, caches game/physics/settings/loadout/debug constants, and supports controlled setting and saving. `SubGameConfig` provides read-only scoped access for weapon definitions.

The configuration currently contains `audio`, `debug`, `game`, `loadout`, `physics`, `settings`, `sounds`, `units`, and `weapons` sections. Avoid duplicating names, IDs, balance values, spawn data, or other canonical metadata in code when configuration or definitions already own them.

Missile death effects keep `instantRadius` and `explosionStartRadius` independent: the former controls the model-less one-frame damage pulse, while the latter controls the visible explosion's initial radius. As a rule of thumb, missiles with a large instant-damage radius should usually start their visible explosion near that radius; small missiles may use a smaller start radius to preserve gradual visual growth.

## Build Commands

Run these through WSL from the `project/` directory:

```sh
make all
make run
make test
make all_test
make test-unit
make test-integration
make test-smoke
make testbin
make test-manual-bin
make test-manual TEST=mouse_test
make clean
make fclean
make re
```

The workspace-level Makefile delegates `make run` to `project/`. The project Makefile discovers application sources and test-category sources automatically. `make test` and `make all_test` run only deterministic unit, integration, and smoke tests. `make test-manual TEST=<basename>` is the explicit single-manual-program runner; `make test-manual-bin` attempts to build all manual programs without launching them, and legacy API drift may make this best-effort target fail.

## Current Tests and Target TDD Structure

The current `tests/manual/` files are standalone programs. Several open raylib windows, wait for user interaction, or render visual output. Most do not contain automated assertions; `test_weapon_strength.cpp` prints derived weapon calculations. They are useful experiments and visual checks, but they are not a deterministic unit suite. Some manual programs currently have API drift and may not compile; do not repair them as part of automated test infrastructure.

Catch2 v3.15.0 is installed under `includes/catch2/`. The active test taxonomy is:

```text
tests/
├── unit/          Fast, deterministic, headless Catch2 tests
├── integration/   Deterministic tests across real project modules
├── smoke/         Automated bounded startup/resource/system checks
└── manual/        Interactive rendering, input, camera, and visual tools
```

`tests/unit/basic_utils_test.cpp` currently covers `merge_vectors`. `tests/smoke/catch2_runner_smoke_test.cpp` verifies that the Catch2 runner is wired. `tests/integration/` is intentionally empty until a real cross-module scenario exists. The six legacy interactive programs are under `tests/manual/`.

## Code Style

Prefer failure-first control flow:

```cpp
void processEntity(GameContext &context, entt::entity entity)
{
    if (!context.registry.valid(entity))
        return;

    auto *position = context.registry.try_get<Position>(entity);
    if (position == nullptr)
        return;

    updatePosition(*position);
}
```

Keep signatures stable, use accurate names, avoid deep nesting, centralize shared concepts, and keep functions focused. Be especially careful with early returns in frame and simulation loops.

## Performance and Maintainability

Do not go and optimize existing code unless explicitly requested in chat. Always aim for optimization when adding new code. For new code, consider ECS iteration shape, allocation, data locality, rendering/audio work, collision complexity, copies, and synchronization without adding speculative complexity.

Large application-owned files currently include `battlefield_hud_renderer.cpp` (~838 lines), `sound_manager.cpp` (~508), `bullet_weapon.cpp` (~364), `components.hpp` (~355), `renderer.cpp` (~355), `missile_weapon.cpp` (~345), `system_player_move_control.cpp` (~287), and `lazer_weapon.cpp` (~258). These are context signals, not authorization for refactoring.

## Known Documentation Drift

The legacy root `summary.md`, `guides/Context.md`, and `.github/copilot-instructions.md` are not the canonical tracked context. `guides/Context.md` still describes reset and the gameplay loop as living in `main.cpp`; current code places them in `Game::reset()` and `Game::run()`. Historical terminology also mentions a physics interpolation file that is not present; current movement integration is `ecs_systems::processMoveRequest`.
