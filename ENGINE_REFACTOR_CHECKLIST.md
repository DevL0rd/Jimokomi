# Engine Refactor Checklist

This checklist now reflects the final code-side cleanup that actually landed. The goal was one authoritative structure per concern, no compatibility bridge layer, and no fragmented external content format for static game definitions.

## Rules

- [x] Replace overloaded files with focused modules instead of adding more branches to them
- [x] Keep static definitions in code where that is clearer than data-file fragmentation
- [x] Keep runtime save/load on generic snapshots of live state
- [x] Keep gameplay entities as concrete assembly points instead of mini-engines
- [x] Remove old naming/layout leftovers instead of preserving them for compatibility

## 1. WorldObject Split

- [x] Keep `WorldObject` as the core runtime primitive
- [x] Move snapshot import/export into `src/primitives/world_object/WorldObjectSnapshot.lua`
- [x] Move collision/overlap/destroy event emission into `src/primitives/world_object/WorldObjectEvents.lua`
- [x] Keep transform/collider access centralized through `WorldObject`
- [x] Re-check primitive identities after the split

## 2. Collision Split

- [x] Create `src/classes/collision/CollisionTileQueries.lua`
- [x] Create `src/classes/collision/CollisionShapeTests.lua`
- [x] Create `src/classes/collision/CollisionBroadphase.lua`
- [x] Create `src/classes/collision/CollisionResolver.lua`
- [x] Create `src/classes/collision/CollisionEvents.lua`
- [x] Rewrite `Collision.lua` as the orchestration point over those modules
- [x] Move entity pair collection onto a bucketed broadphase path
- [x] Re-check world-query and raycast boundaries after the split

## 3. Transform / Scene Split

- [x] Split transform position logic into `src/classes/scene/TransformPosition.lua`
- [x] Split slot storage/rules into `src/classes/scene/TransformSlots.lua`
- [x] Split attachment lifecycle into `src/classes/scene/TransformAttachments.lua`
- [x] Keep transform snapshotting in `src/classes/scene/TransformSnapshot.lua`
- [x] Reduce `Transform.lua` to a thin scene-facing assembly point

## 4. Engine Split

- [x] Move service bootstrapping into `src/classes/engine/EngineServices.lua`
- [x] Move layer creation/ordering into `src/classes/engine/EngineLayers.lua`
- [x] Move update/draw/start/stop orchestration into `src/classes/engine/EngineLoop.lua`
- [x] Move engine debug helpers into `src/classes/engine/EngineDebug.lua`
- [x] Move engine snapshot logic into `src/classes/engine/EngineSnapshot.lua`
- [x] Reduce `Engine.lua` to top-level assembly only

## 5. Layer Split

- [x] Move bucket registration into `src/classes/layer/LayerBuckets.lua`
- [x] Move add/remove entity lifecycle into `src/classes/layer/LayerObjects.lua`
- [x] Move layer snapshot import/export into `src/classes/layer/LayerSnapshot.lua`
- [x] Keep `Layer` focused on service ownership plus entity registry
- [x] Keep player ownership and map ownership explicit

## 6. Player Split

- [x] Split `Jiji` input into `src/classes/player/PlayerController.lua`
- [x] Split locomotion orchestration into `src/classes/player/PlayerLocomotion.lua`
- [x] Split inventory/equipment interaction into `src/classes/player/PlayerLoadout.lua`
- [x] Split visual selection into `src/classes/player/PlayerVisuals.lua`
- [x] Keep `src/entities/Jiji.lua` as the concrete player assembly point

## 7. World Query Split

- [x] Split bounds logic into `src/classes/world_query/WorldQueryBounds.lua`
- [x] Split visibility tests into `src/classes/world_query/WorldQueryVisibility.lua`
- [x] Split tile/surface lookup into `src/classes/world_query/WorldQueryTiles.lua`
- [x] Split random sampling/rule resolution into `src/classes/world_query/WorldQuerySampling.lua`
- [x] Split rule builders into `src/classes/world_query/WorldQueryRules.lua`
- [x] Reduce `WorldQuery.lua` to a thin world-facing assembly point

## 8. Raycast Split

- [x] Split shape intersection into `src/classes/raycast/RaycastShapes.lua`
- [x] Split tile stepping into `src/classes/raycast/RaycastTiles.lua`
- [x] Split entity filtering/hit selection into `src/classes/raycast/RaycastEntities.lua`
- [x] Reduce `Raycast.lua` to a small orchestration point

## 9. Graphics Split

- [x] Split primitive drawing into `src/classes/graphics/GraphicsPrimitives.lua`
- [x] Split procedural texture dispatch into `src/classes/graphics/GraphicsProcedural.lua`
- [x] Split gradient/pattern helpers into `src/classes/graphics/GraphicsGradients.lua`
- [x] Keep `Graphics.lua` as a small renderer assembly point

## 10. Runtime State and Static Definitions

- [x] Keep serializable runtime state separate from static code-defined rules
- [x] Keep save/load on generic snapshot tables instead of per-type content schemas
- [x] Keep static game definitions in code where that is clearer than fragmented data files
- [x] Re-check `AssetRegistry` so code-defined config stays clean and readable

## 11. Agent / Gameplay Primitive Split

- [x] Split relations/target evaluation into `src/primitives/agent/AgentRelations.lua`
- [x] Split action/state helpers into `src/primitives/agent/AgentActions.lua`
- [x] Split spawn/eat lifecycle into `src/primitives/agent/AgentSpawning.lua`
- [x] Keep `Agent.lua` focused on actor assembly

## 12. Item Primitive Split

- [x] Split stack behavior into `src/primitives/Item/item/ItemStacks.lua`
- [x] Split world interactions into `src/primitives/Item/item/ItemInteractions.lua`
- [x] Split visual ownership hooks into `src/primitives/Item/item/ItemVisuals.lua`
- [x] Keep `Item.lua` focused on item assembly
- [x] Keep `Consumable` building on `Item` without content-file indirection

## 13. Debug System Split

- [x] Split line formatting into `src/classes/debug/DebugOverlayLines.lua`
- [x] Split object inspection into `src/classes/debug/DebugOverlayObjects.lua`
- [x] Split summary/performance rendering into `src/classes/debug/DebugOverlaySummary.lua`
- [x] Add a performance-focused debug line to the overlay
- [x] Keep debug overlays out of the hot path when disabled

## 14. Final Cleanup

- [x] Remove dead code left behind by these splits
- [x] Remove transitional helpers that only existed to bridge old structure
- [x] Re-scan include paths and rename leftovers
- [x] End on one direct structure with no backwards-compat layer in the refactored areas

## Progress Notes

- Collision, transform, layer, engine, world query, raycast, graphics, agent, item, and debug responsibilities are now split into focused modules.
- Static game definitions remain in code via `GameData.lua`; runtime state remains snapshot-driven.
- Runtime verification is the next step in Picotron, but the code-side refactor checklist is complete.
