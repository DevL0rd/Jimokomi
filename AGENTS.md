# Project Rules

## Core Rules
- Use `Scene + Entity + Component`.
- `Scene` owns world lifecycle and systems.
- `Entity` is a lightweight composition container.
- `Component` holds focused state only.
- Systems own behavior.
- Keep one source of truth per subsystem.
- Delete dead paths instead of layering replacements on top.

## Design Rules
- Prefer small, focused modules.
- One responsibility per file.
- Group by subsystem, not convenience.
- Keep APIs few and obvious.
- Prefer explicit ownership and boundaries.
- Do not add arbitrary preemptive limits, caps, throttles, thresholds, budgets, or fallback cutoffs.
- Any limit must be required by a real invariant or exposed through centralized config/settings with a clear owner and default.
- If a performance optimization needs a threshold, make it configurable or derive it from measured system state; do not bury magic numbers in subsystem code.
- Prefer designs that make invalid states and failure paths impossible instead of relying on defensive fallbacks.
- Do not keep backward-compatibility shims, legacy behavior, or rescue paths unless they are still intentionally required.
- Prefer standard names and standard patterns.
- Follow DRY.
- Prefer composition over inheritance-heavy designs.
- Avoid vague utility buckets unless truly justified.

## Architecture Prevention Rules
- Before adding code, name the subsystem owner and responsibility in plain terms.
- Before creating a new file, helper, API, cache, queue, index, or state struct, search the existing subsystem for an owner that already does the job.
- Check neighboring modules, private headers, public callers, and existing profiling/diagnostic paths before writing new subsystem code.
- If a piece already exists in the wrong place, move it to the right owner instead of duplicating it.
- Add new behavior to the file/module that owns that responsibility; if no owner exists, create a focused owner instead of extending a broad file.
- Public subsystem files should orchestrate lifecycle and route public APIs; private modules should own storage, caches, queues, indexing, layout, queries, and execution details.
- Do not create partial splits where the new file is only a helper while the original file still owns multiple unrelated responsibilities.
- A split is only valid when the behavior, private state, private header, lifecycle ownership, and call sites move together.
- Keep private state grouped by responsibility instead of exposing one large shared internal struct to every module.
- Prefer real compiled modules with focused private headers over include-fragment `.c` files.
- Do not add copied replacement `.c` files. Move ownership in the same change and delete the old path.
- Before a file becomes the place for a second unrelated subsystem concern, stop and split around actual ownership.
- When a subsystem needs stats, diagnostics, or profiling, keep those paths attached to the subsystem ownership instead of creating detached observability-only splits.
- Preserve one indexing, invalidation, caching, or queueing path per subsystem. Do not add parallel membership lists or duplicate truth paths.
- Delete dead paths in the same change that replaces them; do not leave aliases, compatibility shims, rescue paths, or duplicate implementations.

## Subsystem Boundary Rules
- Resource systems must keep registry, cache, queue, admission, invalidation, loading/execution, and stats ownership explicit.
- Debug overlay systems must keep input, layout, history, dashboard, inspector, world drawing, cache/signatures, stats, and dispatch ownership explicit.
- Physics systems must keep world lifecycle, bodies, shapes, queries, tilemap integration, contacts/snapshots, and task integration ownership explicit.
- Scene systems must keep lifecycle/update orchestration separate from storage, component indexes, factories, view/bounds, queries, and stats/accessors.
- Runtime systems must keep render-thread input, packet serialization, simulation application, render loop, simulation loop, resources, and profiler ownership explicit.
- Rendering backends must keep window/input, render targets/surfaces, primitive drawing, sprite drawing, and text rendering ownership explicit.
- Renderer internals must keep frame building, sorting, visibility, instancing, backdrop handling, and draw dispatch ownership explicit when those areas grow.

## API Boundary Rules
- Public headers should expose stable subsystem APIs, not private exchange objects, builder internals, storage layouts, or implementation-only getters.
- Re-check public callers before expanding broad headers such as scene, physics, resource, renderer, or snapshot APIs.
- Prefer focused public headers for distinct public responsibilities when a broad header keeps growing.
- Game code must not include runtime/rendering internals such as snapshot exchanges, resource command queues, backend types, task/thread primitives, or debug overlay internals.

## Engine Rules
- Rendering is engine-owned, not game-owned.
- Physics wrappers stay engine-facing; physics truth stays in the physics engine.
- Shared immutable render resources should be reused through resource handles.
- Baked surfaces are for cacheable visuals; cheap transient primitives stay live.
- Cache invalidation must be driven by visual state, not transform state.
- Position should not trigger rebakes.
- Rotation/scale should only trigger rebakes when pixels truly change.

## Profiling Rules
- Always expand profiling when a gap appears.
- Treat missing profiling as a bug in observability.
- Profile low-level and high-level paths.
- Keep runtime logging and profiler output clean and readable.
- Prefer real counters, timings, and state metadata over guesses.
- Treat frame drops and pacing changes as first-class performance signals.

## Build Rules
- Replace recursive source globs with explicit grouped source lists when touching build structure.
- New source files must be intentional build decisions, not accidental compilation through recursive globs.
- Keep source-list grouping aligned with subsystem ownership.

## Consistency Rules
- Use explicit, industry-standard terminology.
- Keep naming clinical and searchable.
- Keep config centralized in simple code-owned config files.
- Prefer defaults that are safe, reusable, and easy to reason about.
- If a system becomes clever but hard to explain, simplify it.
