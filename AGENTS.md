# Project Instructions

## Read First

Before broad exploration, read [docs/project-context.md](docs/project-context.md). Follow [docs/development-protocols.md](docs/development-protocols.md) for planning, approval, implementation, testing, and recovery.

For model or asset pipeline work, read [docs/model-asset-conventions.md](docs/model-asset-conventions.md).
For procedural model design, implementation, benchmarking, and screenshot QC, read [procedural model generation workflow](docs/workflows/procedural-model-generation.md).
For asteroid asset work and repeated-render benchmarks, also read [procedural asteroid generator and rendering evidence rules](docs/specs/procedural-asteroid-and-wsl-rendering.md)


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

## C++ and ECS Rules

- Use guard clauses and early returns instead of nested `if/else` blocks.
- Check failure conditions first and return early.
- Be careful with early returns inside frame and simulation loops: required timer, particle, camera, audio, cleanup, or state updates must still run.
- Keep high-frequency simulation and rendering state out of UI-style abstractions.
- Runtime entities should primarily hold identity and mutable state. Canonical metadata belongs in definitions; behavior belongs in systems, strategies, or focused utilities.
- A user-facing concept must have one canonical definition. Do not independently hardcode names, labels, IDs, aliases, balance values, resistances, capabilities, or spawn metadata.
- Keep systems explicit about their required component views and update order.
- When unsure of usage and API, grep includes/entt/entt.hpp directly

## Testing

- Catch2 v3.15.0 is the repository's installed test framework.
- Follow Red-Green-Refactor for behavioral code.
- Bug fixes start with a regression test. Risky legacy changes start with characterization tests.
- Unit tests must be deterministic, bounded, and headless: no window, GPU, audio device, user input, network, or asset pack requirement.
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
