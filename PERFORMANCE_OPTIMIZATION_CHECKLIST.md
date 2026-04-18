# Performance Optimization Checklist

Goal: make the engine blazing fast based strictly on current profiler data, with the highest-impact hot paths handled first.

## Plateau Read

Recent profiler passes show a clear pattern:
- update-side work improved a lot
- draw-side work keeps concentrating into a few buckets instead of disappearing
- repeated small tweaks to animation rates / guide counts / one-off paths are giving smaller and smaller returns

So the next phase should avoid low-signal churn and focus on only the remaining structural bottlenecks:
- shared procedural surface churn
- attachment sprite traversal
- debug-on draw overhead
- collision/contact solving

## Current Working Theory

What is likely true now:
- the engine is no longer losing most of its time to broad miscellaneous waste
- it is losing time to a few expensive families that multiply by instance count:
  - procedural frames
  - rotation buckets
  - attachment sprite trees
  - debug guide assembly
- cache behavior matters more than before because the hottest visuals are heavily shared and heavily reused

What is likely not worth chasing right now:
- more auto-cache logic
- micro-tuning random object classes without a profiler reason
- weakening stress visuals just to get a prettier number

Current profiler shape:
- draw is the bigger bottleneck than update
- biggest draw costs:
  - `layer.render.entities`
  - `layer.render.entities.procedural_sprite`
  - `layer.render.entities.debug_object`
  - `engine.debug_overlay.draw`
  - `attachment_nodes`
- biggest update costs:
  - `layer.sim.update_entities`
  - `layer.sim.resolve_collisions`
  - `Glider`
  - `game.pathing.rebuild`
  - `world.pathfinder.find_path`

## Tier 1: Biggest Wins

- [ ] Reduce `layer.render.entities`
  - [ ] Flatten hot entity draw paths further
  - [ ] Remove unnecessary wrapper/dispatch work in the hottest families
  - [ ] Make the common case as close to direct stamp/draw as possible

- [ ] Reduce `layer.render.entities.procedural_sprite`
  - [ ] Audit `SpaceBall` rotated/frame cache reuse
  - [ ] Audit `Water` shared cache reuse
  - [ ] Keep shared rotated/frame surfaces alive long enough to avoid rebuild churn
  - [ ] Reuse temporary rotated/procedural surfaces where possible
  - [ ] Avoid any per-instance work layered on top of otherwise shared procedural visuals

- [ ] Reduce `layer.sim.resolve_collisions`
  - [ ] Tighten broadphase candidate filtering
  - [ ] Skip sleeping-vs-sleeping and static-vs-static as early as possible
  - [ ] Reduce stale persistent contact work
  - [ ] Keep locked-rotation fast paths cheap

- [ ] Reduce `layer.render.entities.attachment_sprite_nodes`
  - [ ] Specialize the single-sprite case
  - [ ] Specialize tiny flat sprite lists
  - [ ] Avoid recursing through generic node machinery when the subtree shape is already known
  - [ ] Cache subtree family/shape information and invalidate only on node topology change

## Tier 2: High-Value Followups

- [ ] Reduce `layer.sim.update_entities`
  - [ ] Audit `Glider` update behavior first
  - [ ] Cache or skip no-op state transitions
  - [ ] Reduce repeated AI/path/target recomputation

- [ ] Reduce `layer.render.entities.debug_object`
  - [ ] Keep debug visuals to simple lines/circles only
  - [ ] Avoid any expensive per-object debug branching
  - [ ] Avoid cache work for tiny debug primitives unless proven useful

- [ ] Reduce attachment render overhead
  - [ ] `layer.render.entities.attachment_nodes`
  - [ ] Skip structural-only nodes sooner
  - [ ] Cache subtree visibility/family info per frame
  - [ ] Keep sprite-only trees out of the mixed generic path

- [ ] Reduce `engine.debug_overlay.draw`
  - [ ] Keep panel cheap and direct
  - [ ] Reduce guide collection/assembly churn
  - [ ] Cache within-frame guide geometry where helpful
  - [ ] Treat debug as a separate perf budget and stop it from stealing wins from gameplay/render work

## Tier 3: Medium Priority

- [ ] Reduce pathfinding spikes
  - [ ] `game.pathing.rebuild`
  - [ ] `world.pathfinder.find_path`
  - [ ] Rebuild only when start/goal/topology materially changed
  - [ ] Reuse recent paths when still valid

- [ ] Reduce update cost for `Fly` and `Worm`
  - [ ] Push offscreen throttling harder where safe
  - [ ] Skip expensive work when distant or inactive

- [ ] Tighten cache footprint
  - [ ] Prune rotated/frame surface variants more aggressively
  - [ ] Reduce stale debug/panel cache entries
  - [ ] Keep `cache_surface_entries` from creeping up under stress

## Tier 4: Outlier Audits

- [ ] Audit hottest update objects
  - [ ] `WorldObject:Glider#4`
  - [ ] `WorldObject:Glider#2`
  - [ ] `WorldObject:Glider#1`

- [ ] Audit hottest draw objects
  - [ ] `WorldObject:Cherry#22`
  - [ ] hottest visible `SpaceBall` instances
  - [ ] `Water`

## Specific Implementation Ideas

- [ ] Procedural surface pooling
- [ ] Rotated-surface variant eviction by recency
- [ ] Per-frame memoization for attachment subtree classification
- [ ] Per-frame memoization for guide geometry
- [ ] Simplify panel rendering if caching is not paying for itself
- [ ] Reduce debug draw cost before touching non-debug systems when debug is on
- [ ] Audit `Cherry` draw path for accidental debug/rotation overhead

## Benchmark Protocol

- [ ] Keep one stable stress scene and do not weaken it during tuning
- [ ] Compare against the last genuinely better window, not just the immediately previous run
- [ ] Treat these as the primary scoreboard:
  - [ ] `engine.draw`
  - [ ] `layer.render.entities`
  - [ ] `layer.render.entities.procedural_sprite`
  - [ ] `layer.render.entities.attachment_sprite_nodes`
  - [ ] `layer.sim.resolve_collisions`
  - [ ] `cache_entries`
  - [ ] `cache_surface_entries`
- [ ] Reject tweaks that only move one small bucket while making the total frame worse

## Stop Doing

- [ ] Stop making content-level compromises unless the user asks
- [ ] Stop random micro-tweaks to fps/frames/count caps unless the profiler clearly points there
- [ ] Stop changing multiple visual behaviors when the goal is engine throughput
- [ ] Stop assuming a local improvement is real unless `engine.draw` or `engine.update` also improves

## Not Priority Right Now

- [ ] Do not spend more time on auto-cache heuristics
  - current logs show `auto_known: 0`
  - this is not the bottleneck anymore

- [ ] Do not spend more time on chunk query systems right now
  - sound / LOS / raycast are not major hotspots in current data

- [ ] Do not spend more time on base map rendering right now
  - map cost is present but not the main problem

## Success Criteria

- [ ] `engine.draw` drops substantially below current windows
- [ ] `layer.render.entities` is no longer the dominant cost
- [ ] `procedural_sprite` cost is materially lower
- [ ] `resolve_collisions` is materially lower
- [ ] debug-on mode stays readable without becoming a major bottleneck
- [ ] cache entry count stays stable and bounded under stress
