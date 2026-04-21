#include "Game/GameInternal.h"

#include "../Engine/Rendering/ResourceManager.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void game_emit_profiler_metadata(
    GameState* game,
    const RenderSnapshotBuffer* render_snapshot,
    const RendererFrame* frame,
    size_t drained_resource_commands,
    uint64_t dropped_render_snapshots,
    const ResourceCommandQueue* resource_command_queue
) {
    RendererStatsSnapshot renderer_stats;
    DebugOverlayStatsSnapshot debug_stats;
    SceneStatsSnapshot scene_stats;
    SpatialGridStatsSnapshot spatial_stats;

    if (game == NULL || render_snapshot == NULL || frame == NULL) {
        return;
    }

    renderer_get_stats_snapshot(game->renderer, &renderer_stats);
    debug_overlay_get_stats_snapshot(renderer_get_debug_overlay(game->renderer), &debug_stats);
    if (game->scene != NULL) {
        Scene_GetStatsSnapshot(game->scene, &scene_stats);
        Scene_GetSpatialGridStatsSnapshot(game->scene, &spatial_stats);
    } else {
        memset(&scene_stats, 0, sizeof(scene_stats));
        memset(&spatial_stats, 0, sizeof(spatial_stats));
    }

    EngineProfiler_set_metadata_number(&game->engine.profiler, "world_width", WORLD_WIDTH);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "world_height", WORLD_HEIGHT);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "render_items", (double)frame->item_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "ball_count", (double)BALL_COUNT);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_workers", (double)task_system_get_worker_count(&game->task_system));
    EngineProfiler_set_metadata_number(&game->engine.profiler, "visible_candidates", (double)render_snapshot->world.item_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "cull_grid_ms", render_snapshot->stats.cull_grid_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "cull_grid_candidates", (double)render_snapshot->stats.cull_grid_candidates);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "cull_preload_screens", SPATIAL_QUERY_PRELOAD_SCREENS);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_span_min_x", (double)render_snapshot->stats.cull_grid_span_min_x);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_span_max_x", (double)render_snapshot->stats.cull_grid_span_max_x);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_span_min_y", (double)render_snapshot->stats.cull_grid_span_min_y);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_span_max_y", (double)render_snapshot->stats.cull_grid_span_max_y);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_span_cells", (double)render_snapshot->stats.cull_grid_span_cells);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_entries", (double)spatial_stats.entry_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_nodes", (double)spatial_stats.node_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_rebuild_ms", spatial_stats.rebuild_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_incremental_ms", spatial_stats.incremental_update_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_query_ms", spatial_stats.query_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_query_cells", (double)spatial_stats.query_cells);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_query_hits", (double)spatial_stats.query_hits);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_query_false_positives", (double)spatial_stats.query_false_positives);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_dirty_cells", (double)spatial_stats.dirty_cell_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_incremental_dirty_entities", (double)spatial_stats.incremental_dirty_entity_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_full_rebuilds", (double)spatial_stats.full_rebuild_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spatial_grid_incremental_updates", (double)spatial_stats.incremental_update_count);
    EngineProfiler_set_metadata_number(
            &game->engine.profiler,
            "spatial_grid_rebuild_non_empty_cells",
            (double)spatial_stats.rebuild_non_empty_cells
        );
    EngineProfiler_set_metadata_number(
            &game->engine.profiler,
            "spatial_grid_rebuild_max_cell_entries",
            (double)spatial_stats.rebuild_max_cell_entries
        );
    EngineProfiler_set_metadata_number(
            &game->engine.profiler,
            "spatial_grid_rebuild_hottest_cell_ms",
            spatial_stats.rebuild_hottest_cell_ms
        );
    EngineProfiler_set_metadata_number(
            &game->engine.profiler,
            "spatial_grid_query_max_cell_visits",
            (double)spatial_stats.query_max_cell_visits
        );
    EngineProfiler_set_metadata_number(
            &game->engine.profiler,
            "spatial_grid_query_hottest_cell_ms",
            spatial_stats.query_hottest_cell_ms
        );
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_substeps", (double)render_snapshot->stats.physics_substeps);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_ms", render_snapshot->stats.overlay.physics_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "sim_ms", render_snapshot->stats.overlay.sim_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_total_bodies", (double)render_snapshot->stats.overlay.total_body_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_sleeping_bodies", (double)render_snapshot->stats.overlay.sleeping_body_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_moved_bodies", (double)render_snapshot->stats.overlay.moved_body_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_active_entities", (double)render_snapshot->stats.physics_active_entities);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_dirty_entities", (double)render_snapshot->stats.physics_dirty_entities);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_collider_changed_entities", (double)render_snapshot->stats.physics_collider_changed_entities);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_body_create_queue", (double)render_snapshot->stats.physics_body_create_queue);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_body_remove_queue", (double)render_snapshot->stats.physics_body_remove_queue);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_shape_change_queue", (double)render_snapshot->stats.physics_shape_change_queue);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "sim_input_ms", render_snapshot->stats.sim_input_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "sim_spawn_ms", render_snapshot->stats.sim_spawn_ms);
    EngineProfiler_set_metadata_number(
        &game->engine.profiler,
        "sim_fixed_step_wall_ms",
        render_snapshot->stats.sim_fixed_step_wall_ms
    );
    EngineProfiler_set_metadata_number(&game->engine.profiler, "sim_drag_ms", render_snapshot->stats.sim_drag_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "sim_snapshot_acquire_ms", render_snapshot->stats.sim_snapshot_acquire_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "sim_snapshot_build_ms", render_snapshot->stats.sim_snapshot_build_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "scene_input_route_ms", scene_stats.input_route_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "scene_random_force_ms", scene_stats.random_force_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "scene_physics_sync_ms", scene_stats.physics_sync_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "scene_spatial_grid_ms", scene_stats.spatial_grid_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "scene_camera_follow_ms", scene_stats.camera_follow_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "render_alpha", render_snapshot->stats.render_alpha);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_hz", render_snapshot->stats.physics_hz);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "physics_step_substeps", (double)render_snapshot->stats.physics_step_substeps);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "render_snapshot_sequence", (double)render_snapshot->sequence);
    EngineProfiler_set_metadata_number(
            &game->engine.profiler,
            "render_snapshot_age_ms",
            render_snapshot->published_at_ms > 0U && frame->now_ms >= render_snapshot->published_at_ms
                ? (double)(frame->now_ms - render_snapshot->published_at_ms)
                : 0.0
        );
    EngineProfiler_set_metadata_number(&game->engine.profiler, "render_snapshot_dropped", (double)dropped_render_snapshots);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "tracked_ball_index", (double)render_snapshot->stats.tracked_ball_index);
    EngineProfiler_set_metadata_number(
            &game->engine.profiler,
            "selected_ball_index",
            game->selected_ball_index == INVALID_INDEX ? -1.0 : (double)game->selected_ball_index
        );
    EngineProfiler_set_metadata_bool(&game->engine.profiler, "drag_active", render_snapshot->stats.drag_active);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "player_x", render_snapshot->stats.player_x);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "player_y", render_snapshot->stats.player_y);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "player_vx", render_snapshot->stats.player_vx);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "player_vy", render_snapshot->stats.player_vy);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "camera_x", render_snapshot->stats.camera_x);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "camera_y", render_snapshot->stats.camera_y);
    {
            ResourceManagerStatsSnapshot resource_stats;
            resource_manager_get_stats_snapshot(renderer_get_resource_manager(game->renderer), &resource_stats);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_visual_sources", (double)resource_stats.visual_source_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_materials", (double)resource_stats.material_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_shaders", (double)resource_stats.shader_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_baked_surfaces", (double)resource_stats.baked_surface_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_prebake_budget", (double)resource_stats.bake_budget_per_frame);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_pending_bakes", (double)resource_stats.pending_bake_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_static_pending_bakes", (double)resource_stats.static_pending_bake_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_transient_pending_bakes", (double)resource_stats.transient_pending_bake_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_bake_interest_entries", (double)resource_stats.bake_interest_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_bake_requests_this_frame", (double)resource_stats.bake_requests_this_frame);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_dirty_visual_sources", (double)resource_stats.dirty_visual_source_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_dirty_materials", (double)resource_stats.dirty_material_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_dirty_shaders", (double)resource_stats.dirty_shader_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_dirty_baked_surfaces", (double)resource_stats.dirty_baked_surface_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_bake_cache_hits", (double)resource_stats.bake_cache_hits);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_bake_cache_misses", (double)resource_stats.bake_cache_misses);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_invalidation_visual_sources", (double)resource_stats.bake_invalidation_visual_source_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_invalidation_materials", (double)resource_stats.bake_invalidation_material_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_invalidation_shaders", (double)resource_stats.bake_invalidation_shader_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_invalidation_animation_frames", (double)resource_stats.bake_invalidation_animation_frame_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_prebake_ready_visual_sources", (double)resource_stats.prebake_ready_visual_source_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_prebake_ready_materials", (double)resource_stats.prebake_ready_material_count);
            EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_baked_surface_bytes", (double)resource_stats.baked_surface_memory_bytes);
    }
    EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_command_drained", (double)drained_resource_commands);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_command_enqueued_total", (double)render_snapshot->stats.resource_commands_enqueued);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "resource_command_dropped_total", (double)render_snapshot->stats.resource_commands_dropped);
    EngineProfiler_set_metadata_number(
            &game->engine.profiler,
            "resource_command_deduped_total",
            (double)resource_command_queue_get_deduped_count(resource_command_queue)
        );
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_last_items", (double)renderer_stats.render_item_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_last_sprites", (double)renderer_stats.sprite_draw_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_visible_items", (double)renderer_stats.visible_item_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_overlay_draws", (double)renderer_stats.overlay_draw_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_procedural_items", (double)renderer_stats.procedural_item_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_instanced_batches", (double)renderer_stats.instanced_batch_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_instanced_draws", (double)renderer_stats.instanced_draw_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_sort_ms", renderer_stats.sort_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_visibility_ms", renderer_stats.visibility_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_body_ms", renderer_stats.body_draw_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_overlay_ms", renderer_stats.overlay_draw_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "renderer_instance_ms", renderer_stats.instance_draw_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_visible_entities", (double)debug_stats.visible_entity_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_active_collisions", (double)debug_stats.active_collision_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "player_neighborhood_count", (double)game->last_player_neighborhood_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "ball_transform_dirty_aggregate", (double)scene_stats.spatial_dirty_entity_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spawn_region_dirty_count", (double)game->spawn_region_dirty_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spawn_region_dirty_x", (double)game->last_spawn_region_dirty_x);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "spawn_region_dirty_y", (double)game->last_spawn_region_dirty_y);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "hover_pick_queries", (double)game->hover_pick_query_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "hover_pick_skips", (double)game->hover_pick_skip_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "hover_query_dirty", (double)game->hover_query_dirty_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "camera_rect_dirty", (double)game->camera_rect_dirty_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "camera_rect_stable", (double)game->camera_rect_stable_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_dashboard_redraws", (double)debug_stats.dashboard_redraw_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_dashboard_redraw_skips", (double)debug_stats.dashboard_redraw_skip_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_inspector_redraws", (double)debug_stats.inspector_redraw_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_inspector_redraw_skips", (double)debug_stats.inspector_redraw_skip_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_world_redraws", (double)debug_stats.world_redraw_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_world_redraw_skips", (double)debug_stats.world_redraw_skip_count);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_ui_layout_ms", debug_stats.ui_layout_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_ui_draw_ms", debug_stats.ui_draw_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_ui_composite_ms", debug_stats.ui_composite_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_dashboard_redraw_ms", debug_stats.dashboard_redraw_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_inspector_redraw_ms", debug_stats.inspector_redraw_ms);
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_world_redraw_ms", debug_stats.world_redraw_ms);
    EngineProfiler_set_metadata_number(
        &game->engine.profiler,
        "debug_spatial_grid_bytes",
        (double)spatial_stats.memory_bytes
    );
    EngineProfiler_set_metadata_number(&game->engine.profiler, "debug_overlay_surface_bytes", (double)debug_stats.surface_memory_bytes);
    EngineProfiler_set_metadata_bool(&game->engine.profiler, "debug_enabled", debug_stats.draw_ui);
}
