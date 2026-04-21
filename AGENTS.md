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
- Prefer designs that make invalid states and failure paths impossible instead of relying on defensive fallbacks.
- Do not keep backward-compatibility shims, legacy behavior, or rescue paths unless they are still intentionally required.
- Prefer standard names and standard patterns.
- Follow DRY.
- Prefer composition over inheritance-heavy designs.
- Avoid vague utility buckets unless truly justified.

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

## Consistency Rules
- Use explicit, industry-standard terminology.
- Keep naming clinical and searchable.
- Keep config centralized in simple code-owned config files.
- Prefer defaults that are safe, reusable, and easy to reason about.
- If a system becomes clever but hard to explain, simplify it.
