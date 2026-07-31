# Development Protocols

These protocols apply to humans and coding agents working in this repository.

## 1. Establish Context

1. Read `docs/project-context.md` before broad exploration.
2. Read the source files, tests, headers, and Makefile relevant to the request.
3. State the premises and assumptions behind the requested change.
4. Verify historical claims against current source. If the request conflicts with reality, stop and request a plan revision.

Use preferred host-side `cmd /c` commands for discovery and reading. PowerShell is an allowed fallback when it materially fits the task better. Use patch-based host editing for files. Use WSL for building, running code, testing, and Git.

## 2. Plan Before Tracked Edits

Every tracked change requires a plan and explicit user approval first. This includes documentation, tests, configuration, shaders, build files, and application code.

Store the plan outside this repository at `../scratch/plans/` using:

```text
YYYY-MM-DD_HHmmss_short-kebab-description.md
```

The plan must contain:

- Objective and success criteria.
- Verified premises and assumptions.
- Files in scope and files explicitly out of scope.
- Current and planned Mermaid graphs for non-trivial work.
- Implementation tasks with acceptance and verification criteria.
- Risks, recovery notes, and the expected test strategy.

Approval authorizes only the named scope. A plan may be brief for low-risk work, but it may not be skipped.

## 3. Write-Ahead Mental Simulation

Before editing, describe the intended outcome concisely and simulate:

- Which callers, systems, data flows, and tests change.
- Whether component ownership or update order changes.
- Whether a frame-loop early return could skip required work.
- Whether the new code adds allocations, repeated lookups, unnecessary copies, or hot-path branching.
- How the change will fail and how verification will detect it.

## 4. TDD Protocol

Catch2 v3.15.0 is installed in `includes/catch2/` as the repository-owned test framework.

For behavioral code:

1. Write a focused failing Catch2 test.
2. Run it through WSL and confirm it fails for the intended reason.
3. Implement the smallest approved change.
4. Run the focused test, then the relevant deterministic suite.
5. Refactor only within the approved scope while keeping tests green.

Bug fixes begin with a regression test. Risky changes to existing behavior begin with characterization tests. If TDD is genuinely impractical, document the reason in the approved plan before implementation.

Unit tests must not require a window, GPU, audio device, user input, network, or runtime asset pack. Integration tests may compose real modules. Smoke tests are bounded automated checks with meaningful exit codes. Interactive visual tools are manual tests.

Target test taxonomy:

```text
tests/
├── unit/          Fast, deterministic, headless Catch2 tests
├── integration/   Deterministic tests across real project modules
├── smoke/         Automated bounded startup/resource/system checks
└── manual/        Interactive rendering, input, camera, and visual tools
```

## 5. Performance and Elegance

Do not go and optimize existing code unless explicitly requested in chat. Always aim for optimization when adding new code.

For new or requested code, consider the measured or obvious cost of:

- Per-frame allocations and deallocations.
- ECS view shape, repeated component lookup, and data locality.
- Rendering submissions, model/material work, and audio updates.
- Collision and targeting complexity.
- Copies, conversions, synchronization, and unnecessary indirection.

Prefer the simplest design that meets the requirement. Do not introduce speculative caching, abstractions, parallelism, or micro-optimizations without evidence. Finding an opportunity in existing code is a reportable observation, not implementation authorization.

## 6. Implementation Rules

- Use guard clauses and early returns instead of nested `if/else` blocks.
- Keep signatures stable unless a change is necessary; prefer overloads or focused adapters.
- Keep related behavior colocated and public interfaces narrow.
- Give repeated concepts one canonical owner.
- Keep functions focused and names accurate.
- Do not change unrelated files, reformat broadly, or remove old material accidentally.
- Preserve unrelated dirty work in the worktree.

## 7. Verification

Run commands from WSL, from the repository directory:

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
```

Current Makefile behavior:

- `make test-unit` runs the deterministic unit suite.
- `make test-integration` reports an explicit no-tests message until a real integration test exists.
- `make test-smoke` runs bounded automated smoke checks.
- `make test` and `make all_test` run unit, integration, and smoke categories only.
- `make testbin` builds automated binaries only.
- `make test-manual-bin` attempts to build all manual programs without running them.
- `make test-manual TEST=<basename>` builds and runs exactly one named manual program. Without `TEST`, it lists available names and exits with usage guidance.
- Existing manual programs may have API drift; repair them only under a separate approved maintenance request.

Verification must be proportionate to risk. Documentation changes require path/link checks, consistency checks, and source/Makefile claim verification. Do not claim commands passed unless they were actually run through WSL.

## 8. Scope Control and Recovery

If implementation reveals an adjacent optimization, cleanup, refactor, dependency, or architecture change:

1. Record the observation in the plan or final handoff.
2. Do not implement it unless explicitly requested in chat.
3. If requested, create or revise a plan and obtain approval before continuing.

Update the plan with progress, verification, deviations, and recovery notes. If interrupted, inspect the approved file list and WSL Git status before resuming. Never use destructive recovery commands without explicit authorization.

## 9. Boundaries

Always:

- Verify premises.
- Plan and obtain approval.
- Preserve unrelated work.
- Use Catch2/TDD for behavioral work.
- Consider long-term performance and code elegance for new code.
- Record verification evidence.

Ask first:

- Any tracked edit or scope deviation.
- Existing-code optimization or refactoring.
- Dependency, public API, build, CI, shader, asset, or test-layout changes.
- Deletion or supersession of documentation.
- A TDD exception.

Never:

- Commit secrets, generated artifacts, or build outputs.
- Remove failing tests to obtain a green result.
- Treat manual visual programs as smoke tests.
- Use native Windows tooling for build, run, test, or Git.
- Treat stale summaries as verified architecture.
