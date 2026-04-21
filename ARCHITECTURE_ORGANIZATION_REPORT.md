# Architecture Organization Report

Fresh scan after the current cleanup pass.

Findings are flat. No priority buckets, no scheduling language.

## Completed In This Pass

- `Engine/AppInternal.h` no longer embeds concrete `RaylibBackend` or
  `TaskSystem` layouts.
- `Engine/Runtime/AppRenderLoop.h` no longer includes `AppInternal.h`.
- `Engine/Runtime/AppSimulation.h` no longer exposes
  `EngineAppSimThreadContext`.
- Snapshot exchange APIs moved from `RenderSnapshot.h` to
  `RenderSnapshotExchange.h`.
- `Renderer.c`, `RendererSurfaceBatch.c`, and `RendererVisibility.c` no longer
  include `ResourceManagerInternal.h`.
- `Renderer.c` no longer includes `DebugOverlayInternal.h`.
- `SceneStorage.h` no longer includes `SceneInternal.h`.
- The raylib non-instanced surface-batch path is named as an explicit
  individual draw strategy, not a fallback/rescue path.
- The old resource command queue path remains deleted.

## `Engine/Rendering/ResourceManagerInternal.h`

Problem:

- Resource-manager implementation files still include one broad private header.
- That header exposes registry, baked-surface cache, bake queue, invalidation,
  stats, and backend state to all resource-manager modules.

Why this still violates `AGENTS.md`:

- Private state is grouped, but ownership is not enforced by private header
  boundaries.
- Registry, cache, queue, invalidation, execution, and stats modules can still
  access unrelated private state directly.

Correct organization:

- Split private resource-manager ownership into focused internal headers for
  registry, bake cache, bake queue, invalidation, and stats.
- Keep cross-owner changes behind explicit internal functions.

## `Engine/Scene/SceneInternal.h`

Problem:

- Scene state is grouped, but `SceneInternal.h` still exposes lifecycle, storage,
  physics, tilemap, spatial/view, stats, and camera-follow state together.
- Scene access, factories, queries, storage, view, and systems still include the
  same broad internal header.

Why this still violates `AGENTS.md`:

- The scene split is still partial because private ownership remains centralized
  in one large internal bucket.
- Scene systems can still access unrelated scene responsibilities directly.

Correct organization:

- Give storage, view/bounds, physics integration, stats/accessors, and each
  system the narrow private access they need.
- Keep the full `Scene` private layout out of modules that only need one state
  owner.

## `Engine/Physics/PhysicsWorldInternal.h`

Problem:

- Physics state is grouped, but bodies, queries, tilemap, lifecycle, and snapshot
  modules all include one broad private physics-world header.
- Each physics module can access lifecycle, tilemap, entity tracking, and stats
  directly.

Why this still violates `AGENTS.md`:

- Physics ownership is clearer than before, but still not enforced by private
  boundaries.
- Bodies, queries, tilemap integration, snapshots, and lifecycle remain coupled
  through a shared private state bucket.

Correct organization:

- Split focused private headers around physics lifecycle, body/shape ownership,
  tilemap integration, queries, and snapshot/stats access.
- Keep cross-module writes behind explicit internal functions where ownership
  matters.

## `Engine/Rendering/DebugOverlayInternal.h`

Problem:

- The debug overlay has many implementation files, but every one includes the
  same private header containing UI toggles, layout state, panel caches, history,
  world cache, stats, counters, and rendering helpers.
- Dashboard, inspector, history, input, layout, stats, and world drawing are not
  privately isolated from each other.

Why this still violates `AGENTS.md`:

- The subsystem has file splits, but private state ownership did not move with
  the split.
- Debug overlay rules explicitly call out input, layout, history, dashboard,
  inspector, world drawing, cache/signatures, stats, and dispatch ownership.

Correct organization:

- Split debug overlay private state by dashboard, inspector, history, layout,
  input, world drawing, and stats/cache responsibility.
- Keep shared dispatch state small and route module-specific state through
  focused owners.

## `Engine/Rendering/RendererInternal.h`

Problem:

- Renderer internals still keep camera, resource manager, debug overlay,
  scratch item storage, instance scratch storage, draw counters, timing counters,
  dirty flags, and signatures in one private struct.
- Renderer modules share that whole struct through one internal header.

Why this still violates `AGENTS.md`:

- Renderer internals should keep frame building, sorting, visibility,
  instancing, backdrop handling, and draw dispatch ownership explicit as those
  areas grow.
- Current modules can still read and mutate unrelated renderer state.

Correct organization:

- Split renderer private state around dispatch/lifecycle, signatures, visibility,
  surface batching/instancing, backdrop metadata, and stats.
- Expose narrow internal functions instead of the full renderer layout to every
  renderer module.

## `Engine/Rendering/RenderSnapshot.h`

Problem:

- Snapshot exchange APIs are split out now, but `RenderSnapshot.h` still exposes
  `RenderWorldSnapshot` and `RenderSnapshotBuffer` storage layouts directly.
- Runtime, interaction, and scene snapshot-building code all depend on the same
  DTO storage layout.

Why this still violates `AGENTS.md`:

- Public headers should not expose storage layouts or implementation-only
  handoff structures unless they are intentionally stable API.
- Frame DTO ownership and mutable build-buffer ownership are still mixed.

Correct organization:

- Keep immutable frame-view DTOs separate from mutable snapshot build buffers.
- Move mutable allocation/capacity fields behind render-snapshot internals.

## Clean Checks

- No game files include engine internal headers.
- No empty directories were found under `Engine` or `Game`.
- `CMakeLists.txt` uses an explicit source list instead of recursive source
  globs.
- `./build.sh -j2` passes.
