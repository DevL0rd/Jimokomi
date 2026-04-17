# Performance Execution Progress

Date: 2026-04-18
Owner: Codex
Purpose: persistent implementation checklist for the performance audit so progress survives context loss

Legend:
- `[x]` implemented in code
- `[~]` partially implemented, more code work still planned in this pass
- `[ ]` not implemented yet
- `runtime` means code is done but impact still needs the user's Picotron profile pass

## Critical

- [x] `P1` stop per-update tile span invalidation
- [x] `P2` replace unreliable short-scope timing with trustworthy profiler cost reporting `runtime`
- [x] `P3` separate profiler controls from debug visuals
- [x] `P4` decouple debug visuals from collision interest
- [x] `P5` convert cached draw paths from command replay toward true raster/surface reuse `runtime`

## High

- [x] `P6` gate and reduce debug guide generation
- [x] `P7` reduce collision event bookkeeping overhead after removing debug-driven contact interest `runtime`
- [x] `P8` optimize circle-vs-tile checks
- [x] `P9` reduce broadphase rebuild/string-key churn
- [x] `P10` reduce attachment-node update/draw churn `runtime`
- [x] `P11` add cheaper direct-path acceptance / path rebuild reductions
- [x] `P12` replace linear A* open-list work
- [x] `P13` cache tile query data inside walk/climb/jump searches
- [x] `P14` stop blind full-entity perception scans
- [x] `P15` reduce sound queue linear-scan cost
- [x] `P16` allow raycasts to skip entity loops when only tiles matter
- [x] `P17` reduce offscreen glider ground-sensor cost

## Medium

- [x] `P18` throttle profiler census work
- [x] `P19` slow passive item pickup polling
- [x] `P20` slow edible-agent player distance polling
- [x] `P21` remove attachment sync temp-vector churn
- [x] `P22` reduce small helper allocations in hot loops
- [x] `P23` reduce procedural-sprite draw cost and move stable cases toward raster reuse `runtime`
- [x] `P24` push retained HUD away from command replay `runtime`
- [x] `P25` investigate and optimize stable map draw cost without breaking camera behavior `runtime`

## Follow-Up / Lower Confidence

- [x] `P26` revisit collision resolver ordering and repeated passes `runtime`
- [x] `P27` cache world sampling / spawn-rule query results
- [x] `P28` add practical offscreen raster cache support where runtime-safe `runtime`

## Status

1. All planned code-side items are implemented.
2. Runtime confirmation is intentionally left to the user because Picotron execution was explicitly left in the user's hands.
3. If context is lost, resume with a profile pass against the newest appdata perf log and verify each `runtime` item against live counters and CPU.

## Implementation Notes

1. `P2`: the profiler now ranks scopes with per-scope CPU deltas via `stat(1)` and reports `cpu total/avg/max`; this replaces the useless zeroed short-scope timing signal.
2. `P5`, `P23`, `P24`, `P25`, `P28`: practical surface caching now exists and is used by stable retained panels/HUD, procedural materials, and visible map rendering.
3. `P7`: collision enter/stay/exit bookkeeping now uses stamped persistent contact state instead of rebuilding current/previous maps every update.
4. `P10`, `P21`, `P22`: hot attachment and control paths now reuse state and skip unnecessary offscreen work.
5. `P26`: collision resolution now early-outs when a pass produces no contacts, reducing wasted repeated passes without changing the surrounding structure.
