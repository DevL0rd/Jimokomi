#ifndef JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_H
#define JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_H

#include "Camera.h"
#include "Target.h"
#include "../Core/Input.h"
#include "../Core/Stats.h"

typedef struct DebugEntityView {
    uint64_t id;
    Vec2 position;
    Vec2 velocity;
    float angle_radians;
    float extent_x;
    float extent_y;
    float radius;
    bool is_circle;
    bool is_selected;
    bool is_sleeping;
    bool is_moving;
} DebugEntityView;

typedef struct DebugCollisionView {
    Vec2 point_a;
    Vec2 point_b;
    Vec2 contact_point;
    Vec2 contact_normal;
    bool active;
    bool sensor;
    bool sleeping;
} DebugCollisionView;

typedef struct DebugGridView {
    bool enabled;
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    float cell_size;
    int span_min_x;
    int span_max_x;
    int span_min_y;
    int span_max_y;
} DebugGridView;

typedef struct DebugOverlaySnapshot {
    float fps;
    float update_ms;
    float sim_ms;
    float draw_ms;
    float physics_ms;
    float snapshot_age_ms;
    float physics_hz;
    float render_alpha;
    float physics_pairs_ms;
    float physics_collide_ms;
    float physics_solve_ms;
    uint32_t physics_substeps;
    uint32_t visible_count;
    uint32_t awake_body_count;
    uint32_t total_body_count;
    uint32_t sleeping_body_count;
    uint32_t moved_body_count;
    uint32_t spatial_dirty_cells;
    uint32_t spatial_dirty_entities;
} DebugOverlaySnapshot;

typedef struct DebugOverlay DebugOverlay;

typedef struct DebugOverlayStatsSnapshot {
    size_t visible_entity_count;
    size_t active_collision_count;
    uint32_t dashboard_redraw_count;
    uint32_t dashboard_redraw_skip_count;
    uint32_t inspector_redraw_count;
    uint32_t inspector_redraw_skip_count;
    uint32_t world_redraw_count;
    uint32_t world_redraw_skip_count;
    double dashboard_redraw_ms;
    double inspector_redraw_ms;
    double world_redraw_ms;
    double ui_layout_ms;
    double ui_draw_ms;
    double ui_composite_ms;
    size_t surface_memory_bytes;
    bool draw_ui;
} DebugOverlayStatsSnapshot;

void debug_overlay_init(DebugOverlay *overlay);
void debug_overlay_dispose(DebugOverlay *overlay);
void debug_overlay_get_stats_snapshot(
    const DebugOverlay* overlay,
    DebugOverlayStatsSnapshot* out_snapshot
);
void debug_overlay_draw_world(
    DebugOverlay *overlay,
    RenderBackend *backend,
    uint64_t now_ms,
    const Camera *camera,
    const DebugGridView *grid,
    const DebugEntityView *entities,
    size_t entity_count,
    const DebugCollisionView *collisions,
    size_t collision_count,
    uint64_t selected_entity_id,
    uint64_t hovered_entity_id
);
void debug_overlay_draw_ui(
    DebugOverlay *overlay,
    RenderBackend *backend,
    const DebugOverlaySnapshot *snapshot,
    const EngineStatsSnapshot *stats,
    const DebugEntityView *selected_entity,
    int viewport_width,
    int viewport_height
);
void debug_overlay_toggle(DebugOverlay *overlay);
void debug_overlay_toggle_world_gizmos(DebugOverlay* overlay);
bool debug_overlay_is_ui_visible(const DebugOverlay* overlay);
bool debug_overlay_is_world_gizmos_visible(const DebugOverlay* overlay);
void debug_overlay_handle_input(
    DebugOverlay *overlay,
    const EngineInput *input,
    bool has_selection,
    int viewport_width,
    int viewport_height
);
bool debug_overlay_is_pointer_over_ui(
    DebugOverlay *overlay,
    bool has_selection,
    float screen_x,
    float screen_y,
    int viewport_width,
    int viewport_height
);

#endif
