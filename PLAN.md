# Water System Plan

## Goal
- Add a real engine-level 2D volumetric water feature.
- Use `Scene + Entity + Component` at the volume level.
- Keep water particles internal to the water system for performance.
- Generate a surface from particle density with `marching squares`.
- Cache the generated surface through the low-level resource system.
- Decouple simulation/update cadence from display refresh rate.

## Terminology
- Use `WaterVolumeComponent`
- Use `WaterSystem`
- Use `marching squares`
- Use `implicit surface extraction`
- Use `generated surface resource` or `generated polygon resource`
- Do not call this `ray marching`

## Architecture Checklist
- [ ] Add `COMPONENT_WATER_VOLUME` to the scene component type list.
- [ ] Create `Engine/Scene/Components/WaterVolumeComponent.h`.
- [ ] Create `Engine/Scene/Components/WaterVolumeComponent.c`.
- [ ] Add `WaterVolumeComponent` to the active engine component set.
- [ ] Keep one ECS component per water body, not one ECS entity per particle.

## Water Volume Component Checklist
- [ ] Store volume bounds.
- [ ] Store particle count.
- [ ] Store particle radius.
- [ ] Store simulation Hz.
- [ ] Store surface update Hz.
- [ ] Store material handle.
- [ ] Store generated surface resource handle.
- [ ] Store tuning parameters:
- [ ] gravity scale
- [ ] damping
- [ ] jitter / brownian motion
- [ ] cohesion / smoothing
- [ ] contour threshold
- [ ] grid cell size
- [ ] Add dirty/revision tracking for generated surface refresh.

## Water System Checklist
- [ ] Create `Engine/Scene/Systems/WaterSystem.h`.
- [ ] Create `Engine/Scene/Systems/WaterSystem.c`.
- [ ] Update the scene to run `WaterSystem` as part of the main owned systems.
- [ ] Keep internal particle arrays owned by the water system or by each water volume.
- [ ] Do not create full Box2D bodies for all water particles.
- [ ] Update water at a capped simulation rate.
- [ ] Allow a separate capped surface generation rate.

## Particle Simulation Checklist
- [ ] Add internal water particle data structures.
- [ ] Add simple gravity.
- [ ] Add boundary collision against the water volume bounds.
- [ ] Add simple solid collision hooks for nearby world geometry later.
- [ ] Add damping.
- [ ] Add mild brownian motion / jitter.
- [ ] Add neighbor relaxation / cohesion.
- [ ] Keep the first solver simple and stable.

## Surface Extraction Checklist
- [ ] Build a scalar density field from nearby water particles.
- [ ] Sample the field on a 2D grid.
- [ ] Run `marching squares` on that grid.
- [ ] Extract contour loops.
- [ ] Triangulate the contour into a filled polygon.
- [ ] Support multiple contour islands if needed.
- [ ] Track contour revision changes.

## Generated Resource Checklist
- [ ] Add a low-level generated render resource type to the resource system.
- [ ] Keep it consistent with the existing cached/baked resource model.
- [ ] Store:
- [ ] revision
- [ ] dirty flag
- [ ] last update time
- [ ] target update Hz
- [ ] generated geometry or filled-surface payload
- [ ] Add shared lookup through `ResourceManager`.
- [ ] Add cadence-limited regeneration.
- [ ] Reuse previous cached surface if a new one is not needed yet.

## Rendering Checklist
- [ ] Add renderer support for drawing generated water surfaces.
- [ ] Keep it engine-owned, not game-owned.
- [ ] Draw cached generated water every frame.
- [ ] Regenerate only when dirty or when update cadence requires it.
- [ ] Support the normal material/shader path for water.
- [ ] Allow a simple blue default material first.
- [ ] Leave room for future distortion/refraction styling.

## Caching Checklist
- [ ] Make generated water surfaces participate in the low-level cache system.
- [ ] Cache by water volume identity and revision.
- [ ] Separate display FPS from water simulation FPS.
- [ ] Separate water simulation FPS from water surface regeneration FPS.
- [ ] Allow water material animation to run at its own capped rate too.

## Scene Integration Checklist
- [ ] Add a way to spawn a test water volume in the game scene.
- [ ] Add simple tuning values in code first.
- [ ] Keep the water experiment isolated enough to test safely.
- [ ] Avoid exploding the scene into thousands of ECS entities.

## Profiling Checklist
- [ ] Add lightweight engine-side profiling points for:
- [ ] `water_sim_ms`
- [ ] `water_surface_ms`
- [ ] `water_particle_count`
- [ ] `water_grid_resolution`
- [ ] `water_contour_vertex_count`
- [ ] `water_triangulation_ms`
- [ ] `water_cached_surface_count`
- [ ] Treat missing observability here as a bug.

## First Implementation Scope
- [ ] `WaterVolumeComponent`
- [ ] `WaterSystem`
- [ ] internal particles
- [ ] density field
- [ ] marching squares
- [ ] triangulated filled polygon
- [ ] generated cached surface resource
- [ ] simple blue material
- [ ] capped simulation and surface update rates

## Explicit Non-Goals For First Pass
- [ ] Do not build true 3D volumetrics.
- [ ] Do not build a full fluid pressure solver first.
- [ ] Do not make every water particle a Box2D body.
- [ ] Do not add fancy refraction before the base pipeline works.
