# Performance Audit Plan

Date: 2026-04-18
Mode: read, understand, measure, plan only
Status: no code changes proposed yet

## Guardrails

1. Keep the current `Engine -> Layer -> World -> Game` structure intact.
2. Prefer local fixes inside existing modules before any architectural rewrite.
3. Keep profiling concerns separate from debug rendering concerns.
4. Treat measured hotspots as first-class, and label any code-only idea that still needs live confirmation.
5. Do not optimize by piling on overlapping cache systems. One clear cache hierarchy is better than several partial ones.

## Verified Sources

1. Live profiler output from `/home/devl0rd/.lexaloffle/Picotron/drive/appdata/jimokomi_perf_latest.txt`.
2. Main loop and engine services in `main.lua`, `src/Game/Game.lua`, `src/Engine/Core/Engine/Loop.lua`, `src/Engine/Core/Engine/Services.lua`.
3. Update/render flow in `src/Engine/Core/Layer/Simulation.lua` and `src/Engine/Core/Layer/Renderer.lua`.
4. Collision, raycast, pathfinding, perception, attachment-node, and retained-render code paths in the `src/Engine` and `src/Game` trees.

## Important Measurement Notes

1. The `runtime_logs/` files are still empty. The real performance report is currently written to `/appdata/jimokomi_perf_latest.txt` via `src/Engine/Core/Profiler.lua:58` and `src/Engine/Core/Profiler.lua:368-393`.
2. The current scope timing numbers are not trustworthy yet. Every hot scope in the live report is showing `t:0.0 avg:0.0 max:0.0`, which matches the current timing implementation using `time()` around very short scopes in `src/Engine/Core/Profiler.lua:142-175`.
3. The profiler is only enabled when `Engine.debug == true` in `src/Engine/Core/Engine/Services.lua:23-31`, and the game hard-enables debug in `src/Game/Game.lua:50-57`. That means the current performance report is a debug-heavy build, not a clean gameplay baseline.
4. Picotron itself documents `_update()` as `60` calls per second and `_draw()` as variable-rate in `library/picotron.lua:10-15`. The live report matches that: `layer.0.update c:300 hz:60.0` per 5 second window.

## Current Structure Digest

1. `main.lua` installs the game and hands off to `src/Game/Game.lua`.
2. `Game.install()` wires `_init`, `_update`, and `_draw`, and `boot_game()` builds one layer, a party of four gliders, wildlife, pickups, water, and the HUD in `src/Game/Game.lua:50-97`.
3. `EngineLoop.update()` runs layer simulation and profiler reporting in `src/Engine/Core/Engine/Loop.lua:7-37`.
4. `EngineLoop.draw()` clears the frame, draws layers, optionally draws the debug overlay, then records runtime stats in `src/Engine/Core/Engine/Loop.lua:39-100`.
5. Each `Layer` owns `Simulation`, `Renderer`, `Collision`, `Graphics`, `World`, `Camera`, and entity buckets in `src/Engine/Core/Layer.lua`.
6. `Simulation` owns entity update, physics, collision, camera update, and attachment sync in `src/Engine/Core/Layer/Simulation.lua:19-149`.
7. `Renderer` owns map draw, entity draw, and optional debug map labels in `src/Engine/Core/Layer/Renderer.lua:45-158`.
8. `World` owns pathfinding, raycasts, spawning, senses, bounds, and tile queries in `src/Engine/World/World.lua`.
9. Gliders, flies, worms, and pickups all update through the same engine entity loop, with AI/perception mixed into their normal update path through the `Agent` and `Item` mixins.

## Latest Measured Snapshot

Profiler file timestamp checked at `2026-04-18 03:01:18 +0700`.

Across the latest live windows, these patterns stayed consistent:

1. Update remains fixed at `60 Hz` while draw-side stats fluctuate and CPU remains badly over budget.
2. Every 5 second window recorded `>1.0:100`, meaning the sampled CPU stat crossed the over-budget threshold constantly.
3. Stable world size for the current scene:
   - `24` total entities
   - `9` physics entities
   - `9` collidable entities
   - `50` attachment nodes
   - `527` visible map tiles
4. Stable hot counters from the latest windows:
   - `collision.resolver.contacts_processed`: `1987` to `2394`
   - `collision.tiles.circle_checks`: `3023` to `3778`
   - `world.pathfinder.iterations`: average `65.353` to `97.4`, max `160`
   - `layer.nodes.updated`: `2759` to `3218`
   - `layer.nodes.drawn`: `1903` to `2314`
   - `render.cache.replays`: `400`
   - `render.cache.replay_commands`: average `200.74` to `225.4`, max `429` to `513`
   - `render.cache.hits`: `9300` to `12700`
5. The most recent window also surfaced `collision.broadphase.pairs_considered: 630`, which confirms broadphase pair traffic is non-trivial even at only `9` collidable entities.

## Findings And Full Optimization List

### Critical

1. `P1` Solid tile row spans are being invalidated every single update before collision runs.
Evidence: `src/Engine/Core/Layer/Simulation.lua:64-70` calls `setLayerBounds()` every update. `src/Engine/Physics/Collision/TileQueries.lua:44-47` clears `solid_row_spans_by_map`, and `src/Engine/Physics/Collision/TileQueries.lua:49-92` rebuilds the full span cache on demand.
Why it matters: with a `1024x512` world and `16px` tiles, the span builder scans the whole `64x32` tile space before map collision queries can reuse anything. That hidden full-map scan is happening at simulation frequency.
Optimization: only call `setLayerBounds()` when bounds actually change; preserve `solid_row_spans_by_map`; add an explicit map-dirty invalidation path instead of a per-frame reset.
Confidence: verified.

2. `P2` The current profiler cannot time hot scopes accurately, so the optimization loop is partially blind.
Evidence: live report shows every hot scope at `0.0ms`, and the timer uses `time()` in `src/Engine/Core/Profiler.lua:142-175`.
Why it matters: counters are useful, but we cannot rank update/draw subscopes honestly until scope timing is fixed or replaced.
Optimization: separate reliable counters from timing; switch scope timing to a measurement source with useful intra-frame resolution; keep counter reporting either way.
Confidence: verified.

3. `P3` The current performance report is polluted by global debug mode.
Evidence: `src/Game/Game.lua:52-57` forces `Engine.debug = true` and `layer.debug = true`. `src/Engine/Core/Engine/Services.lua:23-31` enables the profiler only when debug is on.
Why it matters: current numbers mix real gameplay work with debug overlay work, debug collision interest, debug guide drawing, and profiler-only census work.
Optimization: split `profiler_enabled`, `debug_overlay_enabled`, and entity-level debug flags into separate controls. Re-profile with debug visuals off before judging final priorities.
Confidence: verified.

4. `P4` Debug mode currently changes collision behavior, not just rendering.
Evidence: `src/Engine/Core/Layer/Objects.lua:3-12` propagates `layer.debug` to every entity. `src/Engine/Objects/WorldObject/Events.lua:26-45` treats `self.debug == true` as collision interest. `src/Engine/Physics/Collision/Broadphase.lua:105-127` keeps contact generation when collision or overlap interest exists.
Why it matters: enabling debug for all entities makes the collision system spend work tracking contacts that gameplay may not need at all.
Optimization: decouple debug visuals from collision interest; require explicit event handlers or explicit debug-contact opt-in before generating contact state.
Confidence: verified.

5. `P5` Cached draw paths are still command replay paths, not true raster reuse.
Evidence: `src/Engine/Rendering/Graphics/Cache.lua:400-411` rebuilds or fetches cached commands, then always calls `replayCommandList()`. `src/Engine/Rendering/Graphics/Retained.lua:193-219` also replays commands every draw in retained mode. Live report shows `render.cache.replays: 400` with `render.cache.replay_commands` averaging `200+` commands per replay.
Why it matters: this reduces rebuild cost, but not the final cost of re-emitting every primitive every draw.
Optimization: first trim replay volume; then investigate whether Picotron supports a real offscreen/raster cache for stable groups. If not, keep one honest retained-command system and stop treating it like a texture cache.
Confidence: verified for current behavior, follow-up required for raster feasibility.

### High

6. `P6` Debug overlay guide generation is rebuilding and replaying a flat command list every draw for every debug entity.
Evidence: `src/Engine/Core/DebugOverlay/Summary.lua:202-211` always calls `drawLayerGuides()` while debug is on. `src/Engine/Core/DebugOverlay/Objects.lua:268-277` rebuilds `commands = {}` and replays it every draw. Because every entity inherits debug, this covers the entire scene.
Why it matters: this is pure draw overhead inside the already overloaded debug build.
Optimization: gate guides separately from summary; only draw guides for selected entities; cache stable guides per entity/state; avoid rebuilding a scene-wide command list every draw.
Confidence: verified.

7. `P7` Collision contact traffic is high enough that the event bookkeeping layer deserves a second pass.
Evidence: latest live report shows `collision.resolver.contacts_processed` in the `1987-2394` range per 5 second window. On top of that traffic, `src/Engine/Physics/Collision/Events.lua:24-124` builds current and previous contact maps for collisions and overlaps every update.
Why it matters: once debug collision interest is removed this contact volume should drop, but today the event layer is still rebuilding state on top of a busy contact stream.
Optimization: remove debug-driven interest first; then re-profile collision event bookkeeping; skip contact-map work for entities with no real enter/stay/exit listeners.
Confidence: verified for the contact-volume signal, verified for the bookkeeping path.

8. `P8` Circle-vs-tile collision checks are frequent and more expensive than they need to be.
Evidence: live report shows `collision.tiles.circle_checks` between `3023` and `3778` per 5 seconds, with `collision.tiles.circle_rows` steady at `5400`. `src/Engine/Physics/Collision/TileQueries.lua:211-267` does a `sqrt()` for each candidate tile on the circle path.
Why it matters: this work runs at update frequency and scales directly with physics entities and crowded tile contact.
Optimization: compare squared distance first; avoid `sqrt()` unless an actual collision is confirmed; cache tile extents or use cheaper separation math for circles against axis-aligned tiles.
Confidence: verified.

9. `P9` Broadphase is rebuilt from scratch every update using temporary tables and string keys.
Evidence: `src/Engine/Physics/Collision/Broadphase.lua:181-208` recreates `buckets` and `visited` every collision pass. Pair keys are concatenated strings in `src/Engine/Physics/Collision/Broadphase.lua:141-147`. Latest live report shows `collision.broadphase.pairs_considered: 630`.
Why it matters: the current scene is small, but this path scales poorly and is already hot enough to surface in the profiler.
Optimization: persist and clear reusable bucket structures; use integer or packed keys instead of strings; only rebuild spatial buckets when an entity crosses cell boundaries.
Confidence: verified.

10. `P10` Attachment-node churn is substantial in both update and draw.
Evidence: live report shows `layer.nodes.updated` between `2759` and `3218` and `layer.nodes.drawn` between `1903` and `2314`. Attachment traversal comes from `src/Engine/Objects/WorldObject/Nodes.lua:23-38` and `src/Engine/Nodes/AttachmentNode.lua:133-158`.
Why it matters: node work is now a recurring hot counter, and the current scene has only `50` attachment nodes.
Optimization: skip unchanged static nodes more aggressively; separate update-only from draw-only nodes; stop walking invisible child chains when parent state guarantees nothing changed.
Confidence: verified.

11. `P11` Pathfinding is expensive enough to show up repeatedly, and some searches are hitting the hard iteration cap.
Evidence: live report shows `world.pathfinder.iterations` averaging `65+` to `97+` with `max:160`. `WalkClimbJump.max_iterations` is `160` in `src/Game/Actors/Locomotion/Pathing/WalkClimbJump.lua:1-3`.
Why it matters: hitting the cap means some searches are exploring as far as the policy allows while still producing uncertain value.
Optimization: add cheaper direct-path acceptance, memoize tile passability/climbability inside each search, reduce search frequency when the target has barely moved, and eventually move to a stronger open-list structure.
Confidence: verified.

12. `P12` The current A* implementation uses linear scans and table churn in the hottest part of the search.
Evidence: `src/Engine/World/Pathfinder.lua:130-145` linearly finds the best open node, `:181-190` linearly updates existing open entries, and `:107-127` plus `:173-198` allocate several tables per search.
Why it matters: even with low actor counts this inflates the cost of every rebuild.
Optimization: replace the open list with a binary heap or bucketed priority queue; reuse search tables; use numeric node keys when possible.
Confidence: verified.

13. `P13` `WalkClimbJump` repeats expensive tile queries and temporary vector creation inside neighbor expansion.
Evidence: `src/Game/Actors/Locomotion/Pathing/WalkClimbJump.lua:22-45`, `:48-95`, and `:128-158` repeatedly call `world:tileToWorld()`, `world:getTileAt()`, and `world:isSolidAt()` while constructing neighbors.
Why it matters: one search can easily perform dozens of repeated tile lookups for the same local area.
Optimization: cache passability and climbability per search; operate directly in tile coordinates instead of converting through world vectors; precompute compact navigation facts for the active map.
Confidence: verified.

14. `P14` Perception still scales as repeated full-entity scans.
Evidence: `src/Game/Mixins/Agent/Perception.lua:114-132` scans every `layer.entities` entry for line-of-sight candidates. The current scene has `24` entities and multiple agents.
Why it matters: this is not the top hotspot today, but it scales with total entities times active agents.
Optimization: add simple faction buckets or spatial buckets; short-circuit to nearby hostile/interesting sets; separate visible, audible, and collectible queries instead of scanning the whole layer each time.
Confidence: verified.

15. `P15` Sound perception still linearly scans the recent sound queue per listener.
Evidence: live report shows `world.senses.sound_queue_size` around `13-17` and `world.senses.sound_candidates` around `3.8-4.5`. `src/Engine/World/World/Senses.lua:112-153` linearly checks the queue.
Why it matters: current counts are manageable, but this becomes an obvious scaling problem once the world gets noisier.
Optimization: bucket sounds by coarse cell or emitter type; cap queue growth more aggressively; keep one latest sound per source/type for AI purposes.
Confidence: verified.

16. `P16` Raycasts still linearly test every collidable entity.
Evidence: `src/Engine/World/Raycast/Entities.lua:47-59` scans `self.layer.collidable_entities`. Live report shows `world.raycast.target_pool: 9`.
Why it matters: the pool is still small, but every new collidable increases sensor and LOS cost linearly.
Optimization: reuse collision buckets for raycast candidate gathering; skip entity tests entirely for sensors that only care about tiles; pre-filter by AABB against the ray segment.
Confidence: verified.

17. `P17` Glider ground sensing continues even offscreen and feeds the raycast path on a timer.
Evidence: `src/Engine/Sensors/RaySensor.lua:5-31` sets `always_update_offscreen = true`. Gliders create a `RaySensor` in `src/Game/Actors/Creatures/Glider/Glider.lua:353-360`, and locomotion reads it through `distToGround()` in `src/Game/Actors/Creatures/Glider/Locomotion.lua:24-35`.
Why it matters: offscreen or inactive party members still contribute repeated sensor work.
Optimization: lower inactive sensor rates further, suspend fully offscreen autonomous gliders when safe, or batch ground probes using tile lookups instead of general raycasts where possible.
Confidence: verified.

### Medium

18. `P18` The simulation profiler itself walks the whole entity list and attachment lists every update just to collect census stats.
Evidence: `src/Engine/Core/Layer/Simulation.lua:73-112` counts particles, procedural sprites, emitters, sprite nodes, and attachments before actual simulation work.
Why it matters: this measurement-only work is added to every debug/profiler frame.
Optimization: move those counts to the 5 second report window, maintain running counters on add/remove, or gate them behind a deeper profiling level.
Confidence: verified.

19. `P19` Many passive world items still do player proximity work every tick.
Evidence: `src/Game/Mixins/Item.lua:48-53` runs `tryTouchPickup()` every update. `src/Game/Mixins/Item/Interactions.lua:123-140` computes distance to the player. Cherries, flies, and worms all pass through item logic.
Why it matters: the cost is modest now, but it is guaranteed per-tick work for a large class of mostly idle entities.
Optimization: move pickup tests to a lower cadence, only test nearby buckets, or let the player query nearby pickups instead of every pickup querying the player.
Confidence: verified.

20. `P20` Edible agents also do direct player distance checks every tick.
Evidence: `src/Game/Mixins/Agent/Spawning.lua:72-111` checks `isEatenByPlayer()` before AI updates.
Why it matters: this duplicates some of the same proximity logic already done for pickup items.
Optimization: unify nearby-interaction checks into one player-centered query pass.
Confidence: verified.

21. `P21` Attachment sync creates unnecessary temporary vectors.
Evidence: `src/Engine/World/Transform/Slots.lua:92-98` allocates a new `Vector` in `getSlotWorldPosition()`. `src/Engine/Nodes/AttachmentNode.lua:141-146` and `src/Engine/World/Transform/Attachments.lua` use sync repeatedly.
Why it matters: this increases GC pressure inside already hot node update paths.
Optimization: write into reusable output vectors or update positions in place without allocating a temporary world-position vector.
Confidence: verified.

22. `P22` Input and path helpers allocate small tables frequently.
Evidence: `src/Game/Actors/Creatures/Glider/Controller.lua` returns fresh input tables, `src/Game/Actors/Creatures/Glider/Autonomy.lua:53-60` creates fresh input state, and path helpers build neighbor/node tables heavily.
Why it matters: these are GC-churn contributors rather than dominant CPU hotspots.
Optimization: reuse input tables, reuse node records where practical, and reduce short-lived helper allocations in the hottest loops.
Confidence: verified as code behavior, lower confidence as a top-level bottleneck.

23. `P23` Procedural sprites are still command-heavy even when cached.
Evidence: `src/Engine/Objects/ProceduralSprite.lua:56-99` caches per animation frame, then draws through `drawCachedLayer()`, which still replays commands. `src/Game/Effects/Materials/Water.lua:20-66` emits many lines and pixels over a `96x56` area. Live report shows `render.procedural_sprite.area: 5376`.
Why it matters: command caching helps, but animated procedural materials still create a non-trivial draw floor.
Optimization: reduce primitive count for water, lower animation frame count, or switch stable procedural layers to a true raster cache if the runtime can support it.
Confidence: verified.

24. `P24` Retained HUD rendering is organized well but still participates in command replay every draw.
Evidence: `src/Game/Actors/Creatures/Glider/Hud.lua:55-259` builds a retained tree. `src/Engine/Rendering/Graphics/Retained.lua:193-219` replays retained commands each draw.
Why it matters: HUD is not the first optimization target, but it is still part of the draw baseline.
Optimization: keep the retained structure, but avoid treating retained-command replay as the final cache goal.
Confidence: verified.

25. `P25` Map rendering draws `527` visible tiles every draw, but the profiler cannot yet tell us how expensive the native `map()` call is relative to everything else.
Evidence: live report consistently shows `layer.render.visible_tiles` and `layer.render.drawn_tiles` at `527`. `src/Engine/Core/Layer/Renderer.lua:48-81` issues one `map()` call per draw.
Why it matters: this is clearly a stable draw cost, but not yet a proven top offender.
Optimization: only investigate map caching after measurement quality improves and debug overhead is removed.
Confidence: verified as a stable cost signal, low confidence as a priority target.

### Lower Confidence Or Follow-Up Ideas

26. `P26` Collision resolver ordering and multi-pass behavior may still have avoidable work after the bigger fixes land.
Evidence: `src/Engine/Physics/Collision/Resolver.lua:11-58` iterates contacts in three logical passes and repeats the whole collision collection loop per `collision_pass`.
Why it matters: this is worth revisiting after span invalidation and debug-contact inflation are removed.
Optimization: keep it as a second-pass cleanup item, not a first move.
Confidence: code-verified, priority still depends on post-fix profiling.

27. `P27` World sampling and spawn-rule queries may become a problem if respawn-heavy gameplay expands.
Evidence: `src/Engine/World/World/Sampling.lua` and `src/Engine/World/Spawner.lua` still scan bounds and tile positions on demand.
Why it matters: not currently showing up in the live report, but likely to matter once item density and spawn frequency increase.
Optimization: cache spawn candidate sets per rule and map region.
Confidence: code-verified, not currently measured as hot.

28. `P28` Investigating true offscreen raster caches may unlock the biggest draw-side win, but runtime support is still unverified.
Evidence: current code only stores command lists and replays them. No offscreen target or surface reuse is present in the current engine.
Why it matters: if Picotron supports a clean surface capture/blit path, stable groups like HUD, summary panels, and static chrome should stop re-emitting primitives every frame.
Optimization: investigate runtime support only after the current command-replay floor is reduced and measurement quality improves.
Confidence: idea is sound, runtime feasibility still unverified.

## Recommended Execution Order

1. Separate profiling from debug visuals and create a clean baseline.
2. Fix profiler timing so sub-scope durations become trustworthy.
3. Remove the per-update tile-span invalidation in collision.
4. Re-profile and compare contact counts, circle checks, CPU spikes, and draw stability.
5. Remove debug-driven collision interest and debug guide draw work from ordinary profiling runs.
6. Re-profile again, then attack collision broadphase and event bookkeeping with real post-debug numbers.
7. Optimize pathfinding internals and `WalkClimbJump` tile-query churn.
8. Reduce attachment-node churn, offscreen ray sensor cost, and passive pickup polling.
9. Re-evaluate draw-side command replay volume, then decide whether a true raster cache investigation is worth the added complexity.

## Approval Checklist

1. This plan keeps the existing module boundaries intact and aims for local, DRY fixes first.
2. Every critical and high-priority item above is either backed directly by current logs and code or explicitly labeled as a follow-up.
3. No item in this file assumes a capability that has not been verified in the current repo or current runtime report.
