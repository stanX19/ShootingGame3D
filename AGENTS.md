# Project

## Introduction
This is a space shooting game. Design philosophy is realistic physics and sci-fi combat. The fun comes from space simulation, dogfights and tactical gameplay.

## Ambition

### PVP Expansion
Currently it is single player PVE, but the ambition is to make this an online PVP game. Therefore always design systems and write code in a way that is easy to expand into PVP.
- bad: Adding isEnemy or isPlayer component because its easy to do
- good: Use generic faction system, keep it flexible and easy to expand into PVP.

### Multi-threading
Now the mainloop runs in a single thread. But when the time comes, we will decouple rendering, physics, weapons, AI, and other processing into different threads. Always aim for maximum separation of concerns, modularity, and facade.
- bad: A needs to know C, C needs to know A
- good: A can run without C, C can run without A. data sharing happens within ECS, or shared facade B.

# Instructions

## Read First

Before broad exploration, read [docs/project-context.md](docs/project-context.md). Follow [docs/development-protocols.md](docs/development-protocols.md) for planning, approval, implementation, testing, and recovery.

For model or asset pipeline work, read [docs/model-asset-conventions.md](docs/model-asset-conventions.md). 1.0f radius is the unified contract of all 3D models in this game, only game code can scale models afterwards.
For procedural model design, implementation, benchmarking, and screenshot QC, read [procedural model generation workflow](docs/workflows/procedural-model-generation.md).
For asteroid asset work and repeated-render benchmarks, also read [procedural asteroid generator and rendering evidence rules](docs/specs/procedural-asteroid-and-wsl-rendering.md)
For ECS component domains, contracts, and tag conventions, read [docs/components.md](docs/components.md).


## Editing Philosophy

- **Less is More:** If changing 1 line can fix the issue, do not write 50 lines just to make it look "complete".
- **Surgical Edits:** Edits must be minimal, clean, and high-precision. Avoid collateral modifications.
- **Minimal Interference:** Always aim for the minimal change to the codebase. Avoid changing signatures unless absolutely necessary. Use alternative functions or overloads instead when possible.
- **Senior Mindset:** Always suggest senior engineer's best practice for ALL tasks. Think critically on previous conversations; constructive criticism is welcome.
- **Premise Verification:** Before acting on a code edit request, list the underlying premises and assumptions. If an assumption contradicts current codebase reality, inform the user and request a plan revision first.
- **Write Ahead Mental Simulation:** Before making edits, simulate internally what happens after the edit and present a concise high-level outcome to the user.
- You MUST make a plan and seek approval **before starting any tracked edit**.
- **Read first:** Read `docs/project-context.md` before exploring the codebase. Verify its historical claims against current code.
- **Plan First:** Do not modify tracked code, tests, configuration, shaders, build files, or documentation until explicitly approved by the user.
- **Visual Planning:** Plans must include detailed Mermaid graphs of the current status and the status after the planned edit. Store them in workspace-level `scratch/plans/`, outside this repository.
- **Write Ahead Log:** Plans must be written before implementation in `../scratch/plans/`, with a descriptive name and local datetime using `YYYY-MM-DD_HHmmss_short-kebab-description.md`. Update the plan with progress, verification, deviations, and recovery notes.
- **One-File Diff Preview:** Before approval, filesystem writes are limited to the plan and `../scratch/plans/<plan-basename>_diff.patch`. Author the complete unified diff directly as text in that patch. Do not copy or mirror project files, create candidate files, modify existing files, or derive the patch by comparing filesystem contents—including under `scratch/`.
- **Clean Workspace:** Implementation plans, ad hoc scripts, and walkthroughs belong in `../scratch/` and must not be committed.

## Scope and Quality

- Do not go and optimize existing code unless explicitly requested in chat.
- Always aim for optimization when adding new code, with proportionate attention to hot paths, allocations, data locality, ECS iteration, rendering, audio, collision, and algorithmic cost.
- An optimization or refactoring observation is not permission to expand scope. Request it explicitly and obtain a separate approved plan.
- Do not delete old material accidentally. Prefer stable, incremental, surgical edits.
- Do not add dependencies, alter public interfaces, change build/CI configuration, or reorganize tests without explicit approval.
- Preserve unrelated user changes and do not reformat files outside the approved scope.
- Github Copilot will review your actions after every PR.
- Follow SRP, DRY and KISS. Functions must remain focused, readable, and accurately named.
- Examples:
  * bad: forcing model processing logic into project/srcs/entities when its supposed to be ModelManager's scope
  * good: entity code stays stupid calling ModelManager's API, implement in model manager, makes logic reusable (DRY)
  * bad: hardcoding sound fx exclusive to a specific entity when there is an existing sound system
  * good: generalise the sound feature to sound system, the entity is just a caller of the feature
  * bad: adding lots of fields into an existing component when it can be implemented as new component + new system
  * good: Add a new component with target fields, and a new system that view<ExistingComponent, NewComponent>
  * bad: writing a god file just because adding logic to it is least resistance path: god_file.cpp(common_util, feat_a, feat_b)
  * good: DRY, split reusable components into different files, reusable by future codebase: common_util.cpp/hpp, feat_a.cpp/hpp, feat_b.cpp/hpp
  * bad: inventing 10+ systems just to solve one requirement, when a simplified tweak in upstream code can simplify the problem
  * good: clearly consider available options before choosing the approach
  * bad: Renderer manually reading from hudManager every frame, coupling Renderer to hudManager
  * good: Renderer does not know hudManager, main calls hudManager.getRenderingCamera() and passes Camera3D to renderer every round  instead, achieving minimal coupling

## C++ and ECS Rules

> Architecture and code style are **developer automation tooling**: they exist to eliminate friction, automate discovery, enable effortless refactoring, and reduce cognitive load.

### Four Pillars of Automation
- **Automated Symbol Discovery:** Domain namespaces (`ecs_systems::`, `utils::<domain>::`, `entity::`) turn IDE/Clangd autocomplete into instant discovery tools.
- **Automated Global Propagation:** Centralize gameplay constants, arena dimensions, and tuning in `GameConfig.hpp` / `SubGameConfig.hpp` so changes propagate without manual hunt-and-replace; zero magic numbers.
- **Automated Behavior Locality:** 1 feature or domain = 1 `.hpp` + 1 `.cpp` (behavior locality; no god files). Pinpoint and tune logic in isolation with zero collateral damage.
- **Automated Build Tooling:** Makefile autonomously builds missing dependencies (`$(RAYLIB_LIB)`) and tracks pure prerequisites (`$(RAYLIB_LIB)` -> `$(PCH)` -> `$(OBJDIR)/%.o` -> `$(NAME)`) with zero redundant work.

### Code Style & Implementation
- **Naming Conventions:** Types in `PascalCase`; functions, methods, parameters, and locals in `camelCase`; member variables in `m_camelCase` (or `camelCase` for plain data structs); constants in `UPPER_SNAKE`. File names in `snake_case` matching the feature/domain (`system_camera_follow_player.cpp`, `unit_camera.hpp`).
- **Header / Source Hygiene:** Declarations in `.hpp`, definitions in `.cpp`. Prefer `#pragma once` on line 1 of headers. Use forward declarations where pointers/references suffice (light headers compile fast and avoid rebuild cascades).
- **Namespace Formatting:** In `.hpp`, tab-indent declarations inside `namespace` blocks. In `.cpp`, do NOT wrap files in `namespace { }` blocks; explicitly qualify symbol definitions at the definition site (e.g. `void ecs_systems::foo(...)`).
- **Component Cleanliness:** Component structs contain pure ECS data. Nest component-specific helper/POV structs inside the parent component struct. See [docs/components.md](docs/components.md) for canonical domain conventions.
- **Function Cleaness** Anything more than 2 indentation from the parent function should be factored out into its subfunction. NEVER create hell of nested indent! A function should never be more than 50 lines.

### C++ Foundations
- **`const`-Correctness:** Default to `const` on variables, parameters, and return types. Always add `const` until you cannot (documents intent and catches accidental writes at compile time). Mark all non-mutating member functions (especially accessors) `const`. If unsure, add `const` first and correct it only when compiler complains.
  * bad: `float vel = Vector3Length(velPtr->value)`
  * good: `const float vel = Vector3Length(velPtr->value)`
- **Parameter Passing:** Pass small, trivially-copyable types (`int`, `float`, `Vector3`) by value. Pass all other read-only parameters by `const T&` (avoids copies, accepts temporaries/literals).
- **No Assignment in `if` Statements:** Assignment inside `if` statements (such as `if (auto *ptr = ...)`) is strictly banned. Declare the variable explicitly before the `if` statement and check against condition / `nullptr`.
  * bad: `if (auto *prev = context.registry.try_get<PrevPosition>(entity)) { ... }`
  * good: `const auto *prev = context.registry.try_get<PrevPosition>(entity); if (prev != nullptr) { ... }`
- **Multi-Component `try_get`:** When querying multiple optional components for an entity from EnTT, use structured bindings with multi-component `try_get` to avoid sequential calls and nested `if` blocks.
  * bad:
    `const auto *strech = registry.try_get<render::ModelStrech>(entity); if (strech) { const auto *prev = registry.try_get<physics::PrevPosition>(entity); ... }`
  * good:
    `const auto [strech, prev] = registry.try_get<render::ModelStrech, physics::PrevPosition>(entity); if (strech != nullptr && prev != nullptr) { ... }`

### Control Flow & Simulation
- Use guard clauses and early returns instead of nested `if/else` blocks.
- Check failure conditions first and return early.
- Be careful with early returns inside frame and simulation loops: required timer, particle, camera, audio, cleanup, or state updates must still run.
- Keep high-frequency simulation and rendering state out of UI-style abstractions.
- Runtime entities should primarily hold identity and mutable state. Canonical metadata belongs in definitions; behavior belongs in systems, strategies, or focused utilities.
- A user-facing concept must have one canonical definition. Do not independently hardcode names, labels, IDs, aliases, balance values, resistances, capabilities, or spawn metadata.
- Keep systems explicit about their required component views and update order.
- When unsure of usage and API, grep includes/entt/entt.hpp directly

## Visual & UI Palette

- **Canonical Color Palette:** All visual design, HUD elements, UI text, and feedback effects follow a 3-color palette: **Orange**, **Blue**, and **Red**.
  - **Blue (`SKYBLUE` / `BLUE`):** Friendly entities, HUD ring/frame, player notifications/kills, player status, and shields.
  - **Orange (`ORANGE`):** Accents, interactive highlights, warnings, critical hits, and explosions.
  - **Red (`RED`):** Hostiles, damage indicators, danger alerts, and player death.
- Avoid ad-hoc colors (e.g. green) in UI, HUD logs, and telemetry unless explicitly approved.

## Testing

- Catch2 v3.15.0 is the repository's installed test framework.
- Follow Red-Green-Refactor for behavioral code.
- Bug fixes start with a regression test. Risky legacy changes start with characterization tests.
- Unit tests must be deterministic, bounded, and headless: no window, GPU, audio device, user input, network, or asset pack requirement.
- **Headless OpenGL Guard:** Any integration or smoke test that exercises 3D model loading (`ModelManager`, `spaceship::factory`) or Raylib textures must guard with `if (!IsWindowReady()) { SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(64, 64, "headless_test"); }` to avoid `SIGSEGV` null-pointer dereferences in headless WSL/CI environments.
- **No Brittle Change-Detector Tests:** Do not write tests that assert exact literal values of configurable tuning parameters (e.g. `CHECK(config.val == 10.0f)`). Tests must verify **behavior** and systems logic, not configuration file contents. Tuning changes in JSON must never break tests unless the behavioral contract itself is broken.
- Use `tests/unit/`, `tests/integration/`, `tests/smoke/`, and `tests/manual/` as the test taxonomy. Existing interactive visual programs are manual tests, not smoke tests.
- `make test` and `make all_test` run deterministic automated tests only. Use `make test-manual TEST=<basename>` to launch one manual program; use `make test-manual-bin` only when you explicitly want to build all manual programs.
- The integration category currently reports that no tests exist; do not treat that as integration coverage.

## Tool Boundary

- Prefer native Windows `cmd /c` for repository discovery, reading, and other host-side tasks. PowerShell is an allowed fallback when it materially fits the task better.
- Use host-side patch editing for file changes.
- Use WSL for building or running code, testing, and Git. Run those commands from the repository path under `/mnt/c/.../project`.

## Boundaries

### Always

- Verify premises against current source and the Makefile.
- Write and obtain approval for the plan before tracked edits.
- Keep changes within approved files and purpose.
- Review new code for performance, elegance, ownership, testability, and maintainability.
- Run proportionate verification and record evidence in the plan.
- Preserve unrelated changes.

### Ask First

- Any tracked edit or plan deviation.
- Any optimization, cleanup, or refactor of existing code.
- Dependency, public interface, build, CI, shader, asset, or test-layout changes.
- Deleting or superseding documentation.
- Any justified exception to test-first development.

### Never

- Commit secrets or generated/build artifacts.
- Edit vendored dependencies without explicit approval.
- Remove failing tests to make a suite pass.
- Claim a build or test passed without running it through WSL.
- Treat legacy summaries as authoritative without verifying them.
- Turn a discovered optimization opportunity into silent scope expansion.
