# Engine Cleanup Checklist

Goal: make the engine feel professional, consistent, automatic by default, easy for game developers to use, and fast in the hot paths.

## Principles

- One concern per module.
- Engine defaults should own plumbing.
- Game code should describe behavior and content, not engine internals.
- Identical work should share automatically.
- Hot paths should avoid string-heavy branching and unnecessary abstraction.
- Debug and profiling code should be isolated and clearly gated.

## Phase 1: Core Object Model

- [x] Extract `PhysicsBody` engine mixin.
- [x] Extract `ColliderBody` engine mixin.
- [ ] Extract `SpatialEntity` engine mixin.
  - Own `markSpatialDirty()`
  - Own static vs dynamic spatial classification
  - Own spatial dirtiness rules
- [ ] Extract `Lifetime` engine mixin.
  - Own timer/lifetime/expiry logic
- [ ] Extract `AttachmentOwner` engine mixin.
  - Own attachment node add/remove/update/draw/destroy lifecycle
- [ ] Extract `Snapshotable` engine mixin.
  - Own snapshot/export/import/default serialization behavior
- [ ] Extract `DebugDrawable` engine mixin.
  - Own collider/shape debug rendering
  - Own rotation indicators
  - Own debug labels if applicable
- [ ] Make `WorldObject` a thin composition root instead of a broad behavior host.

## Phase 2: Physics and Collision

- [ ] Move remaining sleep/island logic into a dedicated physics-side module.
- [ ] Add explicit body profiles.
  - `static`
  - `dynamic`
  - `dynamic_locked_rotation`
  - `kinematic`
  - `sensor`
- [ ] Add `PhysicsMaterial` concept instead of scattered friction/restitution defaults.
- [ ] Standardize wake reasons.
  - moved
  - contact
  - spawned
  - external impulse
  - forced
- [ ] Separate collision stages more cleanly.
  - broadphase
  - narrowphase
  - persistent contact cache
  - solver
  - events
- [ ] Keep and expand linear-only solver fast paths for locked-rotation bodies.
- [ ] Add clearer collision filtering profiles instead of raw layer/mask setup in game content.
- [ ] Audit map/layer collision response so it matches the pair solver model as closely as practical.

## Phase 3: Rendering and Graphics

- [ ] Keep only two obvious render policies in public engine usage.
  - raw
  - shared cached surface
- [ ] Reduce remaining cache/plumbing ceremony in engine/game call sites.
- [ ] Split debug UI/overlay responsibilities away from generic collider/object modules.
- [ ] Extract `DebugUI` or `OverlayUI` engine module.
- [ ] Make retained/panel identity fully engine-owned by default.
- [ ] Standardize world-space vs screen-space debug draw rules.
- [ ] Audit all procedural sprite users to ensure they rely on automatic shared identities.

## Phase 4: Layer and World Structure

- [ ] Split `Layer` conceptual ownership more clearly.
  - simulation
  - rendering
  - object registry
  - spatial indexes
  - map state
- [ ] Move spatial index management behind a clearer engine service.
- [ ] Keep `World` as the single gameplay query boundary.
- [ ] Unify nearby/query APIs into one coherent world query surface.
  - entities
  - collidables
  - sounds
  - visibility candidates
- [ ] Reduce boot-time manual wiring in `Game.lua` via scene/bootstrap helpers.

## Phase 5: Game-Side Mixins

- [ ] Add `Spawnable` game mixin.
  - respawn
  - spawn rules
  - spawn-state tracking
- [ ] Add `OffscreenThrottled` game mixin.
  - visible = full rate
  - offscreen = interval update
- [ ] Add `TouchablePickup` game mixin.
  - pickup radius
  - touch timer
  - collector checks
- [ ] Add `LocomotionOwner` game mixin.
  - controller + locomotion + state/visual sync glue
- [ ] Slim `Creature` into a thin aggregator over smaller concerns.
- [ ] Slim `Item` into a thin aggregator over smaller concerns.
- [ ] Slim `Agent` into a thin aggregator over smaller concerns.

## Phase 6: Debug and Profiling

- [ ] Keep profiler instrumentation engine-side only.
- [ ] Standardize profiler naming conventions.
- [ ] Add a dedicated physics diagnostics section.
  - sleeping bodies
  - active islands
  - contact counts
  - dynamic vs static chunk counts
- [ ] Add a dedicated query diagnostics section.
  - path budget
  - perception budget
  - spatial query counts
- [ ] Keep overlay legends/help text centralized instead of scattered through feature code.

## Phase 7: Content and Defaults

- [ ] Replace clusters of booleans with engine profiles where possible.
- [ ] Move more defaults from game actors into engine mixins/base classes.
- [ ] Ensure game actors mostly read like:
  - content data
  - gameplay logic
  - visuals
- [ ] Remove redundant engine plumbing from game-side object definitions.

## Highest-Value Next Steps

- [ ] `SpatialEntity`
- [ ] `Lifetime`
- [ ] `AttachmentOwner`
- [ ] `DebugDrawable`
- [ ] `Spawnable`
- [ ] `OffscreenThrottled`
- [ ] slim `Item`
- [ ] slim `Creature`
- [ ] slim `Agent`

## Done When

- `WorldObject` is small and readable.
- Mixins are narrow and obvious.
- Engine handles caching/spatial/debug defaults automatically.
- Game code no longer carries engine boilerplate.
- Debug/profiler systems are isolated and consistent.
- New game objects can be built by composing a few named concerns instead of copying patterns from other actors.
