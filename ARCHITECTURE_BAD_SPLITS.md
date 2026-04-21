# Architecture Bad Splits Audit

This file lists existing files/subsystems that look split badly, incompletely,
or in a way that can become the same kind of cosmetic breakdown we want to
avoid. This is separate from the main cleanup plan: it is an audit of suspicious
boundaries and files to revisit.

## High Priority

### `Engine/Rendering/ResourceManager.c`

Problem:

- Still one 1,500+ line implementation file.
- `ResourceManagerStats.c` is split out, but the main file still owns registry,
  bake cache, bake queues, dirty invalidation, admission policy, bake execution,
  and lifecycle.
- This is a partial split: stats moved, core responsibilities stayed tangled.

Right fix:

- Move registry, bake cache, bake queue, admission, and invalidation into real
  compiled modules with private headers.
- Keep `ResourceManager.c` as lifecycle and public API routing only.

### `Engine/Rendering/DebugOverlay.c`

Problem:

- Still one 1,400+ line implementation file.
- `DebugOverlayStats.c` exists, but the overlay still mixes input, layout,
  history, dashboard drawing, inspector drawing, world gizmos, cached surfaces,
  signatures, and draw dispatch.
- `DebugOverlayInternal.h` exposes one large shared private struct instead of
  grouped state.

Right fix:

- Split into real compiled modules: input, layout, history, dashboard,
  inspector, world, and dispatch.
- Group internal state by responsibility before moving behavior.

### `Engine/Physics/PhysicsWorld.c`

Problem:

- Still one broad 1,000 line Box2D wrapper.
- Owns world lifecycle, body creation/removal, shape sync, tilemap bodies,
  queries, contacts, counters, and task callback integration.
- `PhysicsWorldSnapshot.c` exists, but snapshot work is only one small piece of
  the wrapper.

Right fix:

- Split bodies, shapes, queries, tilemap, task callbacks, and snapshots into
  real modules.
- Keep `PhysicsWorld.c` as public orchestration.

### `Engine/Scene/Scene.c`

Problem:

- Still owns lifecycle, storage, component indexes, random-force component
  insertion, world bounds, tilemap bounds sync, spatial-grid dirty update,
  queries, and destruction.
- `SceneAccess.c` and `SceneFactories.c` exist, but storage/index ownership is
  still mostly in `Scene.c`.

Right fix:

- Split storage, component indexing, view/bounds, queries, stats/accessors, and
  factories into real modules.
- Make one indexing path responsible for add/remove membership.

## Medium Priority

### `Engine/Rendering/RaylibBackend.c`

Problem:

- Large backend file at roughly 900+ lines.
- It likely mixes window lifecycle, event/input capture, surface/render-target
  management, drawing primitives, sprite drawing, and text rendering.

Right fix:

- Split backend input/window handling from render-target/surface management and
  primitive drawing.
- Keep the public backend API stable while reducing file responsibility.

### `Engine/Rendering/Renderer.c`

Problem:

- Mid-large renderer file that likely owns frame building, sorting, visibility,
  instancing, backdrop handling, and draw dispatch.
- Related renderer internals may be clean enough for now, but this should be
  watched before adding more rendering features.

Right fix:

- Split only when there is a clear owner: sorting, visibility, instancing, and
  draw dispatch.
- Avoid a vague renderer utilities bucket.

### `Engine/Scene/SpatialGrid.c`

Problem:

- Large file at 1,100+ lines.
- `SpatialGridDiagnostics.c` is already split out, but the main grid still
  likely owns storage, dirty cell tracking, broadphase query implementations,
  and rebuild/update paths.

Right fix:

- Split storage/rebuild, dirty tracking, and query implementations if the grid
  keeps growing.

### `Engine/Runtime/InteractionSystem.c`

Problem:

- Correctly moved out of scene systems, but still one 400+ line file that spans
  render-thread camera input, hover picking, packet writing, and sim-thread drag
  application.
- This is a better location, not a complete split.

Right fix:

- Split render update, input packet serialization, and sim application into
  focused runtime modules.

## Public Header Concerns

### `Engine/Rendering/RenderSnapshot.h`

Problem:

- Public header exposes `RenderSnapshotExchange`, `RenderWorldSnapshot`, and
  detailed snapshot fields.
- Game code does not currently need most of this.

Right fix:

- Move exchange/builder internals behind private rendering/runtime headers.
- Keep only stable snapshot view types public where needed.

### `Engine/Rendering/ResourceManager.h`

Problem:

- Public API exposes a broad resource surface.
- Some getters may exist mainly because internals are not split cleanly.

Right fix:

- Re-check public callers after the resource manager split.
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

- Split into `Scene.h`, `SceneFactory.h`, `SceneQueries.h`, and possibly
  `ScenePhysics.h` if the public API keeps growing.

## Build Structure

### Root `CMakeLists.txt`

Problem:

- Recursive globs make accidental files compile immediately.
- This makes bad partial splits dangerous: copied source files can introduce
  duplicate symbols or broken half-modules without an explicit source-list
  decision.

Right fix:

- Replace recursive globs with explicit grouped source lists.
- Make new source ownership intentional.

## Current Worktree Warning

The shortcut include-fragment split attempt was reverted. No `ResourceManager`
or `DebugOverlay` include-fragment files should remain.

Do not add new copied `.c` files unless the old source has been updated in the
same change and the build source list is explicit.
