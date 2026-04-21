#ifndef JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_INTERNAL_H

#include "DebugOverlay.h"

#define DEBUG_OVERLAY_HISTORY_CAPACITY 64

typedef struct DebugPanelState {
    float x;
    float y;
    float width;
    float height;
    bool dragging;
    float drag_offset_x;
    float drag_offset_y;
} DebugPanelState;

typedef struct DebugOverlay {
    bool draw_ui;
    bool draw_world_gizmos;
    bool draw_ui_bounds;
    bool layout_initialized;
    bool inspector_collapsed;
    int last_layout_viewport_width;
    int last_layout_viewport_height;
    bool last_layout_has_selection;
    RenderBackend *dashboard_surface_backend;
    Surface *dashboard_surface;
    int dashboard_surface_width;
    int dashboard_surface_height;
    uint64_t last_dashboard_signature;
    uint64_t last_dashboard_redraw_at_ms;
    uint64_t history_serial;
    uint64_t last_history_push_ms;
    float last_one_percent_low_fps;
    RenderBackend *inspector_surface_backend;
    Surface *inspector_surface;
    int inspector_surface_width;
    int inspector_surface_height;
    uint64_t last_inspector_signature;
    uint64_t last_inspector_redraw_at_ms;
    RenderBackend *world_surface_backend;
    Surface *world_surface;
    int world_surface_width;
    int world_surface_height;
    uint64_t last_world_signature;
    DebugPanelState dashboard_panel;
    DebugPanelState inspector_panel;
    size_t last_visible_entity_count;
    size_t last_active_collision_count;
    uint32_t hovered_ui_region;
    uint32_t dashboard_redraw_count;
    uint32_t dashboard_redraw_skip_count;
    uint32_t inspector_redraw_count;
    uint32_t inspector_redraw_skip_count;
    uint32_t world_redraw_count;
    uint32_t world_redraw_skip_count;
    double last_dashboard_redraw_ms;
    double last_inspector_redraw_ms;
    double last_world_redraw_ms;
    double last_ui_layout_ms;
    double last_ui_draw_ms;
    double last_ui_composite_ms;
    float fps_history[DEBUG_OVERLAY_HISTORY_CAPACITY];
    float frame_ms_history[DEBUG_OVERLAY_HISTORY_CAPACITY];
    float update_ms_history[DEBUG_OVERLAY_HISTORY_CAPACITY];
    float sim_ms_history[DEBUG_OVERLAY_HISTORY_CAPACITY];
    float draw_ms_history[DEBUG_OVERLAY_HISTORY_CAPACITY];
    float physics_ms_history[DEBUG_OVERLAY_HISTORY_CAPACITY];
    float visible_count_history[DEBUG_OVERLAY_HISTORY_CAPACITY];
    float display_fps;
    float display_render_ms;
    float display_one_percent_low_fps;
    float display_update_ms;
    float display_sim_ms;
    float display_physics_ms;
    float display_visible_count;
    float display_pairs_ms;
    float display_collide_ms;
    float display_solve_ms;
    float display_physics_hz;
    float display_awake_body_count;
    float display_total_body_count;
    float display_sleeping_body_count;
    float display_moved_body_count;
    float display_spatial_dirty_cells;
    float display_spatial_dirty_entities;
    float display_snapshot_age_ms;
    size_t history_count;
    size_t history_cursor;
} DebugOverlay;

#endif
