#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_H

#include "Camera.h"
#include "RendererConfig.h"
#include "RenderTypes.h"
#include "Target.h"

typedef struct DebugOverlay DebugOverlay;
typedef struct ResourceManager ResourceManager;
typedef struct Renderer Renderer;
struct Scene;

typedef struct RendererStatsSnapshot {
    size_t render_item_count;
    size_t sprite_draw_count;
    size_t visible_item_count;
    size_t overlay_draw_count;
    size_t procedural_item_count;
    size_t baked_surface_count;
    size_t instanced_batch_count;
    size_t instanced_draw_count;
    double sort_ms;
    double visibility_ms;
    double body_draw_ms;
    double overlay_draw_ms;
    double instance_draw_ms;
} RendererStatsSnapshot;

typedef enum RendererDirtyFlags {
    RENDERER_DIRTY_NONE = 0,
    RENDERER_DIRTY_FRAME_LIST = 1 << 0,
    RENDERER_DIRTY_SORT = 1 << 1,
    RENDERER_DIRTY_OVERLAY_LIST = 1 << 2,
    RENDERER_DIRTY_INSTANCE_BATCH = 1 << 3,
    RENDERER_DIRTY_BACKDROP = 1 << 4,
    RENDERER_DIRTY_SNAPSHOT_METADATA = 1 << 5
} RendererDirtyFlags;

Renderer* renderer_create(RenderBackend *backend, const RendererConfig *config);
void renderer_destroy(Renderer* renderer);
void renderer_init(Renderer *renderer, RenderBackend *backend, const RendererConfig *config);
void renderer_dispose(Renderer *renderer);
void renderer_draw(Renderer *renderer, RenderBackend *backend, const RendererFrame *frame);
void renderer_get_stats_snapshot(const Renderer* renderer, RendererStatsSnapshot* out_snapshot);
Camera* renderer_get_camera(Renderer* renderer);
const Camera* renderer_get_camera_const(const Renderer* renderer);
void renderer_get_viewport_size(const Renderer* renderer, int* out_width, int* out_height);
void renderer_toggle_debug_overlay(Renderer* renderer);
void renderer_toggle_debug_world_gizmos(Renderer* renderer);
bool renderer_debug_overlay_is_ui_visible(const Renderer* renderer);
bool renderer_debug_overlay_is_world_gizmos_visible(const Renderer* renderer);
void renderer_debug_overlay_handle_input(
    Renderer* renderer,
    const EngineInput* input,
    bool has_selection,
    int window_width,
    int window_height
);
bool renderer_debug_overlay_is_pointer_over_ui(
    const Renderer* renderer,
    bool has_selection,
    float mouse_x,
    float mouse_y,
    int window_width,
    int window_height
);
void renderer_draw_debug_overlay_ui(
    Renderer* renderer,
    RenderBackend* backend,
    const DebugOverlaySnapshot* snapshot,
    const struct EngineStatsSnapshot* engine_stats,
    const DebugEntityView* selected_entity,
    int window_width,
    int window_height
);

#endif
