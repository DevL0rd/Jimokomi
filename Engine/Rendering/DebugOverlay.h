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

typedef struct DebugOverlaySnapshot {
    float fps;
    float update_ms;
    float draw_ms;
    float physics_ms;
} DebugOverlaySnapshot;

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
    bool enabled;
    bool draw_world_gizmos;
    bool draw_ui_bounds;
    bool layout_initialized;
    RenderBackend *world_surface_backend;
    Surface *world_surface;
    int world_surface_width;
    int world_surface_height;
    uint64_t last_world_redraw_ms;
    uint32_t world_refresh_interval_ms;
    DebugPanelState dashboard_panel;
    DebugPanelState inspector_panel;
    size_t last_visible_entity_count;
    size_t last_active_collision_count;
} DebugOverlay;

void debug_overlay_init(DebugOverlay *overlay);
void debug_overlay_dispose(DebugOverlay *overlay);
void debug_overlay_draw_world(
    DebugOverlay *overlay,
    RenderBackend *backend,
    uint64_t now_ms,
    const Camera *camera,
    const DebugEntityView *entities,
    size_t entity_count,
    const DebugCollisionView *collisions,
    size_t collision_count
);
void debug_overlay_draw_ui(
    DebugOverlay *overlay,
    RenderBackend *backend,
    const DebugOverlaySnapshot *snapshot,
    const EngineStats *stats,
    const DebugEntityView *selected_entity,
    int viewport_width,
    int viewport_height
);
void debug_overlay_toggle(DebugOverlay *overlay);
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
