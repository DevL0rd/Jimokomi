# Architecture Bad Splits Audit

This file tracks architecture splits that are still valid in the current tree.
Resolved or stale entries should be deleted instead of kept as historical noise.

## Medium Priority

### `Engine/Runtime/InteractionSystem.c`

Problem:

- Smaller now, but still mixes render-thread camera/input handling and hover
  picking.
- Packet serialization and sim-thread drag application are already split into
  focused runtime modules.

Right fix:

- Split hover picking into `InteractionPicking.c` if the interaction path grows.
- Keep render-thread input collection in `InteractionSystem.c`.

## Public Header Concerns

### `Engine/Rendering/RenderSnapshot.h`

Problem:

- Public header exposes `RenderSnapshotExchange`, `RenderWorldSnapshot`, and
  detailed snapshot fields.
- Game code does not need most exchange/build internals.

Right fix:

- Move exchange/builder internals behind private rendering/runtime headers.
- Keep only stable snapshot view types public where needed.

### `Engine/Rendering/ResourceManager.h`

Problem:

- Public API still exposes a broad resource surface.
- Some getters may exist mainly because internals were previously not split
  cleanly.

Right fix:

- Re-check public callers after resource manager cleanup.
- Move implementation-only access behind private headers.

### `Engine/Physics/PhysicsWorld.h`

Problem:

- Public API mixes lifecycle, commands, queries, tilemap integration, and
  snapshot access.

Right fix:

- Split command/query/snapshot public APIs or hide tilemap through scene-facing
  APIs.

### `Engine/Scene/Scene.h`

Problem:

- Public scene API mixes lifecycle, entity storage, factories, physics helpers,
  tilemap, world bounds, spatial grid diagnostics, input callback, and queries.

Right fix:

- Split into focused public headers such as `Scene.h`, `SceneFactory.h`,
  `SceneQueries.h`, and possibly `ScenePhysics.h` if the public API keeps
  growing.

## Resolved In Current Cleanup

- `Engine/Rendering/ResourceManager.c` is no longer the old monolith; registry,
  bake cache, bake queue, bake process, invalidation, lifecycle, and stats are
  separate compiled modules.
- `Engine/Rendering/DebugOverlay.c` is no longer the old monolith; input,
  layout, history, dashboard, inspector, panels, world drawing, stats, and
  common helpers are separate compiled modules.
- `Engine/Physics/PhysicsWorld.c` is no longer the old monolith; bodies,
  queries, tilemap, and snapshots are separate compiled modules.
- `Engine/Scene/Scene.c` is no longer the old monolith; access, factories,
  queries, storage, and view logic are separate compiled modules.
- `Engine/Scene/SpatialGrid.c` no longer owns broadphase query implementations;
  AABB, circle, and segment queries live in `SpatialGridQueries.c`.
- `Engine/Rendering/Renderer.c` no longer owns frame/signature hashing;
  signature bookkeeping lives in `RendererSignatures.c`.
- `Engine/Rendering/Renderer.c` no longer owns visibility filtering; culling
  logic lives in `RendererVisibility.c`.
- `Engine/Rendering/Renderer.c` no longer owns baked surface batch preparation
  or flush; batching lives in `RendererSurfaceBatch.c`.
- `Engine/Rendering/RaylibBackend.c` no longer owns instanced surface batching;
  instancing state, shader setup, and batch draw dispatch live in
  `RaylibBackendInstancing.c`.
- Root `CMakeLists.txt` uses an explicit source list; the old recursive-glob
  warning is stale.
