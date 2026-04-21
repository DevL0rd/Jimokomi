# Architecture Cleanup Plan

This is a fresh pass over the current project state after the engine/game split.
It is based on `AGENTS.md`, the current `Engine/`, `Game/`, `main.c`, and
`CMakeLists.txt`.

## North Star

The project should read like this:

- `Game/` defines Jimokomi-specific world setup, rules, spawn data, and visuals.
- `Engine/Scene/` owns entity/component storage, scene lifecycle, and systems.
- `Engine/Physics/` owns the Box2D-facing truth and exposes engine-facing APIs.
- `Engine/Rendering/` owns render resources, render snapshots, debug drawing, and
  backend integration.
- `Engine/Core/` owns platform-neutral services: input, logging, profiling,
  stats, tasking, paths.
- `Engine/App.*` owns app runtime orchestration, but should not become the place
  where every subsystem detail is manually wired forever.

The final game side should stay boring in the best way: register resources,
create a scene, spawn entities, and run the few actual Jimokomi rules.

## Highest Priority Problems

### 1. Engine Runtime Is Better, But `Engine/App.c` Is Too Large

Current file: `Engine/App.c` is about 760 lines and owns:

- backend/window lifecycle
- engine lifecycle
- renderer lifecycle
- task system lifecycle
- input capture and packet publishing
- simulation thread
- fixed timestep accumulator
- render snapshot exchange
- scene snapshot building
- debug overlay input/draw wiring
- resource command draining
- profiler metadata emission
- shutdown ordering

This is engine-owned now, which is correct, but the file has too many
responsibilities. It should be split by runtime subsystem.

Plan:

- Create `Engine/Runtime/` or keep flat names under `Engine/` if desired, but
  group by responsibility:
  - `Engine/App.c`: public `EngineApp_Run` and top-level orchestration only
  - `Engine/AppContext.h`: private app context/state structs
  - `Engine/Runtime/SimulationThread.c/.h`: sim thread loop, fixed timestep,
    input packet consumption, scene update, snapshot publish timing
  - `Engine/Runtime/RenderLoop.c/.h`: main window loop, render snapshot
    acquire/release, frame build, present
  - `Engine/Runtime/AppProfiler.c/.h`: all `EngineProfiler_set_metadata_*`
    calls
  - `Engine/Runtime/AppResources.c/.h`: resource command queue drain and stats
  - `Engine/Runtime/AppInput.c/.h`: backend input snapshot to runtime packet
    translation
- Keep `Engine/App.h` small and public.
- Keep runtime internals private to engine. Game should not include any runtime
  private header.

Acceptance checks:

- `Engine/App.c` drops below about 250 lines.
- No game file mentions `RenderSnapshotExchange`, `InputPacketStream`,
  `ResourceCommandQueue`, `RaylibBackend`, `pthread`, or debug overlay internals.
- Profiler metadata still contains the same or better counters.

### 2. `ResourceManager.c` Needs Subsystem Splitting

Current file: `Engine/Rendering/ResourceManager.c` is about 1,580 lines and owns:

- resource registration
- key lookup
- texture/material/shader/visual-source storage
- baked surface hashing/storage
- dirty tracking
- bake request queues
- bake admission/interest tracking
- bake execution
- stats

This violates small focused modules and makes the render resource model harder
to reason about.

Plan:

- Keep `ResourceManager.h` as the public API, but split implementation:
  - `ResourceManager.c`: init/dispose/frame entry points and public API routing
  - `ResourceRegistry.c/.h`: texture/material/shader/visual source arrays and
    key lookup
  - `BakeCache.c/.h`: baked surface keys, hash slots, get/create/destroy
  - `BakeQueue.c/.h`: static/transient pending bake queues and dedupe slots
  - `BakeInvalidation.c/.h`: dirty handle lists and cache invalidation
  - `BakeAdmission.c/.h`: hit counting and prebake admission thresholds
  - `ResourceManagerStats.c`: keep stats snapshot here; extend if needed
- Move internal structs currently in `ResourceManagerInternal.h` into smaller
  private headers owned by those modules.
- Make invalid states harder:
  - a bake request should not exist with any zero handle
  - a visual source should have exactly one valid kind path
  - dirty lists should dedupe when inserted, not later by defensive scanning

Acceptance checks:

- No single resource-manager implementation file exceeds about 500 lines.
- Public `ResourceManager.h` exposes fewer convenience getters where a stats
  snapshot already covers the need.
- Bake invalidation remains driven by visual/material/shader state, not transform
  state.

### 3. Debug Overlay Needs To Become A Real Subsystem

Current files:

- `Engine/Rendering/DebugOverlay.c` is about 1,400 lines.
- `DebugOverlayStats.c` is tiny and already split out.
- `DebugOverlayInternal.h` exposes one giant private struct.

The overlay is engine-owned and valuable, but it mixes layout, cached surfaces,
input handling, world gizmos, dashboard drawing, inspector drawing, history, and
stats in one file.

Plan:

- Split by responsibility:
  - `DebugOverlay.c`: public init/dispose and high-level draw dispatch
  - `DebugOverlayInput.c`: panel dragging, pointer hit testing, toggles
  - `DebugOverlayLayout.c`: panel layout and viewport clamping
  - `DebugOverlayHistory.c`: history push/read and one-percent-low FPS
  - `DebugOverlayDashboard.c`: dashboard contents only
  - `DebugOverlayInspector.c`: selected entity inspector only
  - `DebugOverlayWorld.c`: world gizmo drawing and world-surface cache
  - `DebugOverlayStats.c`: snapshot and simple toggles
- Replace the one giant internal struct with grouped nested structs:
  - `DebugOverlayPanels`
  - `DebugOverlaySurfaces`
  - `DebugOverlayHistory`
  - `DebugOverlayCounters`
  - `DebugOverlayDisplayValues`
- Keep tweakable overlay values in `Engine/Settings.c`; keep compile-time array
  capacities close to the owning private struct.

Acceptance checks:

- Dashboard, inspector, and world gizmo code can be understood independently.
- Runtime logging/profiler output remains clean.
- Redraw skip/redraw timing counters remain visible.

### 4. Scene Is Still Doing Too Much Entity List Management

Current files:

- `Engine/Scene/Scene.c` is about 940 lines.
- `Engine/Scene/SceneAccess.c` already contains accessors/stats.
- `SceneInternal.h` has many parallel arrays:
  - all entities
  - dynamic entities
  - selectable entities
  - draggable entities
  - renderable entities
  - debug-visible entities
  - trigger/proximity/random-force lists

The ECS shape is good, but the storage bookkeeping is spread through scene code.

Plan:

- Create focused scene storage modules:
  - `SceneStorage.c/.h`: entity array reserve/add/remove/find
  - `SceneComponentIndex.c/.h`: dynamic/selectable/draggable/renderable/random
    force indexes
  - `SceneFactories.c/.h`: helper constructors such as dynamic circle, static
    box, bounds colliders
  - `SceneView.c/.h`: view/world bounds functions
  - `SceneStats.c/.h`: stats snapshot and profiling fields
- Keep `Scene_Update` as the high-level system pipeline only:
  - route input
  - run random force system
  - run physics sync
  - update spatial grid
  - run camera follow
- Revisit list ownership:
  - components should announce index membership through a clear scene indexing
    path
  - avoid manually appending to random-force lists from helper functions if the
    component add path can own it

Acceptance checks:

- `Scene.c` reads as lifecycle + update order, not storage mechanics.
- Adding a new indexed component does not require copying four reserve/append
  patterns.
- Removing an entity from the scene updates every component index through one
  source of truth.

### 5. Physics World Is A Wrapper, But It Is Too Broad

Current file: `Engine/Physics/PhysicsWorld.c` is about 1,000 lines.

It currently owns:

- Box2D world lifecycle
- body/shape creation/removal
- component synchronization
- tilemap collision bodies
- queries
- contacts
- profiling snapshots
- task callbacks

Physics truth should stay in Box2D, and the engine wrapper should stay
engine-facing. The wrapper is right in concept, but needs smaller boundaries.

Plan:

- Split implementation:
  - `PhysicsWorld.c`: init/dispose/update public orchestration
  - `PhysicsBodies.c/.h`: entity body creation/removal and handle resolution
  - `PhysicsShapes.c/.h`: collider-to-shape sync
  - `PhysicsQueries.c/.h`: point/ray/radius/contact queries
  - `PhysicsTilemap.c/.h`: tilemap body generation and rule application
  - `PhysicsSnapshot.c/.h`: snapshot/counter collection
  - `PhysicsTasks.c/.h`: Box2D task integration
- Keep public `PhysicsWorld.h` smaller:
  - separate command APIs from query APIs with obvious grouping
  - hide tilemap APIs behind scene if possible
- Revisit failure paths:
  - avoid silent partial body creation
  - make handle validity checks centralized

Acceptance checks:

- Box2D handles never leak into game code.
- Scene/game code still talks in entities/components.
- Physics profiling counters are preserved or improved.

## Medium Priority Cleanup

### 6. Settings Should Avoid Heavy Rendering Includes

Current issue:

- `Engine/Settings.h` includes `Rendering/RenderTypes.h` just for `Color32`.
- Pulling settings into low-level render backend code can collide with raylib's
  `Camera` typedef because `RenderTypes.h` includes engine rendering types.

Plan:

- Move primitive shared render types such as `Color32`, `Rect`, or other tiny
  value-only types into a lower-level header if appropriate:
  - possible name: `Engine/Core/Color.h`
  - or keep `Color32` in `Core/Geometry.h` if that stays clean
- Make `Settings.h` depend only on core primitive headers and C standard
  headers.
- Keep `RuntimeConfig.c` as the adapter that expands engine settings into
  subsystem configs.

Acceptance checks:

- `Engine/Settings.h` does not include large rendering headers.
- `RaylibBackend.c` can safely include settings if it ever needs to.
- Config remains centralized in simple code-owned files.

### 7. Runtime Config And Settings Overlap

Current files:

- `Engine/Settings.c/.h`
- `Engine/RuntimeConfig.c/.h`
- `Engine/Core/EngineConfig.h`
- `Engine/Rendering/RendererConfig.h`

This is workable, but there are now two layers of defaults. Settings owns
values; runtime config maps them.

Plan:

- Decide the contract:
  - `EngineSettings`: code-owned default values and tweak points
  - `RuntimeConfig`: fully expanded config used to initialize subsystems
- Rename `runtime_config_init_defaults` to something more explicit, such as
  `runtime_config_from_engine_settings`.
- Consider grouping `EngineSettings` fields into nested structs:
  - `EngineWindowSettings`
  - `EngineLoggerSettings`
  - `EngineProfilerSettings`
  - `EngineRendererSettings`
  - `EngineAppSettings`
  - `EngineSceneSettings`
  - `EngineInteractionSettings`
  - `EngineDebugOverlaySettings`
- Keep defaults safe and reusable.

Acceptance checks:

- There is one obvious place to tweak a default.
- The mapping layer is mechanical and short.
- No subsystem hardcodes config that belongs in settings.

### 8. Render Snapshot Stats Need Cleaner Naming

Current issue:

- `RenderStatsSnapshot` contains fields like `sim_spawn_ms`, but the app now
  runs generic game update callbacks, not only spawning.
- It also mixes render, sim, culling, physics, camera, resource command, and
  debug overlay counters in one flat struct.

Plan:

- Rename `sim_spawn_ms` to `sim_game_update_ms`.
- Group stats into nested structs:
  - `RenderSnapshotRenderStats`
  - `RenderSnapshotSimStats`
  - `RenderSnapshotPhysicsStats`
  - `RenderSnapshotCullingStats`
  - `RenderSnapshotResourceStats`
  - `RenderSnapshotCameraStats`
- Update profiler metadata names to match standard terminology.

Acceptance checks:

- Names are clinical and searchable.
- No game-specific wording remains in engine stats.
- Debug overlay and profiler still display all meaningful counters.

### 9. Interaction System Crosses Scene And Rendering

Current file: `Engine/Scene/Systems/InteractionSystem.c`.

It handles camera pan/zoom, hover picking from render snapshots, selection, drag
packet writing, and physics drag application. This is generic engine behavior,
but it straddles scene, rendering, and runtime input.

Plan:

- Move it out of `Engine/Scene/Systems` if it remains cross-subsystem.
  Possible target: `Engine/Runtime/InteractionSystem.*`.
- Split phases:
  - render-thread interaction state update from input + render snapshot
  - runtime packet serialization
  - sim-thread packet application to scene/physics
- Keep selectable/draggable components in scene, but keep interaction orchestration
  at runtime level.

Acceptance checks:

- Scene systems stay scene-local.
- Runtime systems can use render snapshots and backend input without making scene
  own render-thread concepts.

### 10. CMake Uses Recursive Globs

Current `CMakeLists.txt` uses recursive globs for `Game/*.c` and `Engine/*.c`.
This is convenient during churn, but less explicit.

Plan:

- After the next structural refactor settles, replace recursive globs with
  subsystem source lists:
  - `Engine/Core/CMakeLists.txt`
  - `Engine/Rendering/CMakeLists.txt`
  - `Engine/Scene/CMakeLists.txt`
  - `Engine/Physics/CMakeLists.txt`
  - `Game/CMakeLists.txt`
- Keep third-party setup in root CMake.

Acceptance checks:

- Build inputs are explicit and grouped by subsystem.
- Adding/removing files is intentional.

## Game-Side Cleanup

### 11. `Game/GameWorld.c` Still Knows Too Much About Physics Components

Current file uses:

- `PhysicsWorld_GetEntityLinearVelocity`
- `PhysicsWorld_SetEntityLinearVelocity`
- `PhysicsWorld_ApplyEntityLinearImpulse`
- `PhysicsWorld_GetEntityContactCapacity`
- direct `RigidBodyComponent` mutation after spawning

This is not wrong for a simple game, but we can make game code cleaner with
engine-level helpers.

Plan:

- Add engine-facing entity physics helpers:
  - `Scene_GetEntityVelocity`
  - `Scene_SetEntityVelocity`
  - `Scene_ApplyEntityImpulse`
  - `Scene_IsEntityGrounded` or a more general contact query helper
- Add a richer dynamic body spawn descriptor:
  - shape
  - transform
  - render handles
  - body material values
  - initial velocity
  - flags for selectable/draggable/renderable
- Replace direct rigid-body mutation in `jimokomi_spawn_ball` with descriptor
  fields.

Acceptance checks:

- Game spawning reads like data setup.
- Game code does not need to include `RigidBodyComponent.h` just to spawn a ball.
- Physics truth still stays in physics engine.

### 12. `BallVisualResources.c` Is Game-Specific, But Utility Code Can Move

Current file owns procedural ball art and includes a local HSV helper.

Plan:

- Keep Jimokomi-specific ball drawing in `Game/BallVisualResources.c`.
- Move generic color conversion into engine if it is useful elsewhere:
  - `Engine/Rendering/Color.c/.h` or `Engine/Core/Color.c/.h`
  - `Color32_FromHSV`
- Rename local function prefixes from `game_` to `jimokomi_` or
  `ball_visual_` for searchable ownership.
- Replace render size macros with either `static const` values or game config
  fields if they are shared.

Acceptance checks:

- Generic color utility is not duplicated in future visuals.
- Game visual code remains clearly game-owned.

### 13. Remove Stale IDE/File Confusion

`Game/BallVisuals.c` has been replaced by `Game/BallVisualResources.c`. The old
file is deleted from the repo but may still be open in the IDE.

Plan:

- Keep the new file name.
- Do not recreate `BallVisuals.c`.
- Update any docs or comments that mention the old name.

Acceptance checks:

- `grep -R "BallVisuals" Game Engine *.md` returns no real references.

## Public API Cleanup

### 14. Public Headers Should Expose Less Internal Detail

Review these headers:

- `Engine/Rendering/ResourceManager.h`
- `Engine/Rendering/RenderSnapshot.h`
- `Engine/Scene/Scene.h`
- `Engine/Physics/PhysicsWorld.h`
- `Engine/App.h`

Plan:

- Keep public headers for stable engine API only.
- Move builder/exchange internals to private headers where possible:
  - `RenderSnapshotExchange` may not need to be public outside rendering/runtime.
  - `RenderWorldSnapshot` may not need to expose all fields publicly.
- Split large public headers into focused public APIs:
  - `PhysicsWorld.h`
  - `PhysicsQueries.h`
  - `PhysicsSnapshot.h`
  - `Scene.h`
  - `SceneFactory.h`
  - `SceneQueries.h`
- Preserve clear names and avoid vague utility buckets.

Acceptance checks:

- Game includes fewer engine headers.
- Public API is obvious without reading internals.

### 15. Component Defaults Should Come From Settings Or Descriptors

Current component constructors contain defaults such as:

- `RigidBodyComponent`: density/friction/restitution/friction_air
- `ColliderComponent`: default circle radius and rect size
- `RandomForceComponent`: default force strength/interval/seed stride
- `DraggableComponent`: release velocity scale

Plan:

- Decide which are true safe component defaults and which are gameplay defaults.
- Move tweakable engine defaults into settings or descriptor factory values.
- Prefer descriptor constructors for nontrivial components:
  - `RigidBodyComponent_CreateWithDesc`
  - `ColliderComponent_CreateCircle`
  - `RenderableComponent_CreateWithHandles`
  - `DraggableComponent_CreateWithDesc`

Acceptance checks:

- Component constructors do not hide surprising gameplay behavior.
- Game constants stay in `Game/GameConfig.h`.
- Engine defaults stay in `Engine/Settings.c`.

## Observability And Profiling

### 16. Keep Expanding Profiling During Splits

Current profiling is broad and useful. Do not regress it while splitting files.

Plan:

- Preserve all existing profiler metadata during refactors.
- Add timings around newly split responsibilities:
  - resource queue drain
  - interaction render update
  - interaction sim apply
  - snapshot acquire/publish wait
  - debug overlay layout/draw/composite already exists and should remain
- Keep profiler names stable unless renaming for clarity.

Acceptance checks:

- Missing profiling for a new low-level path is treated as a bug.
- Runtime logs and profiler text remain readable.

## Suggested Execution Order

1. Rename and regroup render snapshot stats.
2. Add scene/entity physics helper APIs so game world loses direct rigid-body
   mutation.
3. Split `Engine/App.c` into runtime modules.
4. Move `InteractionSystem` to runtime and split render/sim phases.
5. Split `DebugOverlay.c`.
6. Split `ResourceManager.c`.
7. Split `PhysicsWorld.c`.
8. Split scene storage/index/factory code out of `Scene.c`.
9. Lighten public headers and settings includes.
10. Replace recursive CMake globs with explicit subsystem source lists.

## Ongoing Checks

Run these after each phase:

```sh
cmake --build build -j
grep -R "GameInputPacket\|BallInteraction\|GameInternal\|BallVisuals" -n Engine Game *.md
grep -R "RaylibBackend\|InputPacketStream\|RenderSnapshotExchange\|ResourceCommandQueue\|pthread" -n Game Engine/App.h
grep -R "selected_ball\|tracked_ball\|player_" -n Engine
```

Expected direction:

- `Game/` keeps getting smaller and more declarative.
- Engine public APIs get smaller and more intentional.
- Large files shrink by subsystem, not by moving code into vague utility files.
- Profiling coverage stays as good or better after every refactor.
