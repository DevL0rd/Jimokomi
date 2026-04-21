#include "AppProfiler.h"

#include "../Core/Profiler.h"
#include "../Rendering/DebugOverlay.h"
#include "../Rendering/RenderSnapshot.h"
#include "../Rendering/ResourceCommandQueue.h"
#include "../Rendering/ResourceManager.h"
#include "../Scene/Scene.h"

void engine_app_emit_profiler_metadata(
    EngineAppContext* app,
    const RenderSnapshotBuffer* render_snapshot,
    const RendererFrame* frame,
    size_t drained_resource_commands
)
{
    RendererStatsSnapshot renderer_stats;
    DebugOverlayStatsSnapshot debug_stats;
    SceneStatsSnapshot scene_stats;
    SpatialGridStatsSnapshot spatial_stats;
    ResourceManagerStatsSnapshot resource_stats;

    if (app == NULL || render_snapshot == NULL || frame == NULL)
    {
        return;
    }

    renderer_get_stats_snapshot(app->renderer, &renderer_stats);
    debug_overlay_get_stats_snapshot(renderer_get_debug_overlay(app->renderer), &debug_stats);
    Scene_GetStatsSnapshot(app->scene, &scene_stats);
    Scene_GetSpatialGridStatsSnapshot(app->scene, &spatial_stats);
    resource_manager_get_stats_snapshot(renderer_get_resource_manager(app->renderer), &resource_stats);

    EngineProfiler_set_metadata_number(&app->engine.profiler, "render_items", (double)frame->item_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "visible_candidates", (double)render_snapshot->world.item_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "cull_grid_ms", render_snapshot->stats.culling.cull_grid_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "cull_grid_candidates", (double)render_snapshot->stats.culling.cull_grid_candidates);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_entries", (double)spatial_stats.entry_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_nodes", (double)spatial_stats.node_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_rebuild_ms", spatial_stats.rebuild_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_incremental_ms", spatial_stats.incremental_update_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_query_ms", spatial_stats.query_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_query_cells", (double)spatial_stats.query_cells);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_query_hits", (double)spatial_stats.query_hits);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_dirty_cells", (double)spatial_stats.dirty_cell_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_substeps", (double)render_snapshot->stats.physics.physics_substeps);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_ms", render_snapshot->stats.overlay.physics_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_ms", render_snapshot->stats.overlay.sim_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_total_bodies", (double)render_snapshot->stats.overlay.total_body_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_sleeping_bodies", (double)render_snapshot->stats.overlay.sleeping_body_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_moved_bodies", (double)render_snapshot->stats.overlay.moved_body_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_input_ms", render_snapshot->stats.sim.input_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_game_update_ms", render_snapshot->stats.sim.game_update_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_fixed_step_wall_ms", render_snapshot->stats.sim.fixed_step_wall_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_drag_ms", render_snapshot->stats.sim.drag_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_snapshot_acquire_ms", render_snapshot->stats.sim.snapshot_acquire_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_snapshot_build_ms", render_snapshot->stats.sim.snapshot_build_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_input_route_ms", scene_stats.input_route_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_random_force_ms", scene_stats.random_force_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_physics_sync_ms", scene_stats.physics_sync_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_spatial_grid_ms", scene_stats.spatial_grid_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_camera_follow_ms", scene_stats.camera_follow_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "render_alpha", render_snapshot->stats.render.render_alpha);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_hz", render_snapshot->stats.physics.physics_hz);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "render_snapshot_sequence", (double)render_snapshot->sequence);
    EngineProfiler_set_metadata_number(
        &app->engine.profiler,
        "render_snapshot_age_ms",
        render_snapshot->published_at_ms > 0U && frame->now_ms >= render_snapshot->published_at_ms
            ? (double)(frame->now_ms - render_snapshot->published_at_ms)
            : 0.0
    );
    EngineProfiler_set_metadata_number(&app->engine.profiler, "render_snapshot_dropped", (double)app->dropped_render_snapshots);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "camera_x", render_snapshot->stats.camera.camera_x);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "camera_y", render_snapshot->stats.camera.camera_y);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_visual_sources", (double)resource_stats.visual_source_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_materials", (double)resource_stats.material_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_shaders", (double)resource_stats.shader_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_baked_surfaces", (double)resource_stats.baked_surface_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_pending_bakes", (double)resource_stats.pending_bake_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_command_drained", (double)drained_resource_commands);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_command_enqueued_total", (double)render_snapshot->stats.resources.resource_commands_enqueued);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_command_dropped_total", (double)render_snapshot->stats.resources.resource_commands_dropped);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_command_deduped_total", (double)resource_command_queue_get_deduped_count(app->resource_command_queue));
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_last_items", (double)renderer_stats.render_item_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_last_sprites", (double)renderer_stats.sprite_draw_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_visible_items", (double)renderer_stats.visible_item_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_instanced_batches", (double)renderer_stats.instanced_batch_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_instanced_draws", (double)renderer_stats.instanced_draw_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_sort_ms", renderer_stats.sort_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_visibility_ms", renderer_stats.visibility_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_body_ms", renderer_stats.body_draw_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_overlay_ms", renderer_stats.overlay_draw_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_instance_ms", renderer_stats.instance_draw_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "debug_visible_entities", (double)debug_stats.visible_entity_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "debug_active_collisions", (double)debug_stats.active_collision_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "hover_pick_queries", (double)app->interaction_state.hover_pick_query_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "hover_pick_skips", (double)app->interaction_state.hover_pick_skip_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "hover_query_dirty", (double)app->interaction_state.hover_query_dirty_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "camera_rect_dirty", (double)app->interaction_state.camera_rect_dirty_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "camera_rect_stable", (double)app->interaction_state.camera_rect_stable_count);
    EngineProfiler_set_metadata_bool(&app->engine.profiler, "debug_enabled", debug_stats.draw_ui);
}
