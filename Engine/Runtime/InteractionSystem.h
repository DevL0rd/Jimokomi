#ifndef JIMOKOMI_ENGINE_SCENE_SYSTEMS_INTERACTIONSYSTEM_H
#define JIMOKOMI_ENGINE_SCENE_SYSTEMS_INTERACTIONSYSTEM_H

#include "../Core/Input.h"
#include "../Core/Geometry.h"
#include "../RuntimeInput.h"

#include <stdbool.h>
#include <stdint.h>

struct RenderSnapshotBuffer;
struct Renderer;
struct Scene;

typedef struct InteractionSystemConfig
{
    float camera_pan_key_speed;
    float camera_zoom_step;
    float camera_zoom_key_speed;
    float camera_pan_click_threshold;
    float camera_min_view_width;
    float camera_min_view_height;
} InteractionSystemConfig;

typedef struct InteractionSystemState
{
    uint64_t selected_entity_id;
    uint64_t hovered_entity_id;
    uint64_t dragged_entity_id;
    uint64_t last_hover_snapshot_sequence;
    int last_hover_mouse_x;
    int last_hover_mouse_y;
    float last_hover_camera_x;
    float last_hover_camera_y;
    float last_hover_camera_view_width;
    float last_hover_camera_view_height;
    bool last_hover_pointer_over_ui;
    bool hover_cache_valid;
    bool entity_drag_active;
    Vec2 drag_world_position;
    Vec2 drag_release_velocity;
    bool camera_pan_active;
    int camera_pan_start_mouse_x;
    int camera_pan_start_mouse_y;
    float camera_pan_start_x;
    float camera_pan_start_y;
    uint32_t hover_pick_query_count;
    uint32_t hover_pick_skip_count;
    uint32_t hover_query_dirty_count;
    uint32_t camera_rect_dirty_count;
    uint32_t camera_rect_stable_count;
} InteractionSystemState;

void InteractionSystem_ConfigDefaults(InteractionSystemConfig* config);
void InteractionSystem_Init(InteractionSystemState* state);
void InteractionSystem_UpdateRender(
    InteractionSystemState* state,
    const InteractionSystemConfig* config,
    struct Renderer* renderer,
    const EngineInput* input,
    const struct RenderSnapshotBuffer* render_snapshot,
    bool pointer_over_ui,
    double dt_seconds
);
void InteractionSystem_WriteInputPacket(
    const InteractionSystemState* state,
    const EngineInputSnapshot* input_snapshot,
    const struct Renderer* renderer,
    bool pointer_over_ui,
    bool debug_overlay_enabled,
    bool draw_debug_world,
    int window_width,
    int window_height,
    uint64_t frame_id,
    EngineRuntimeInputPacket* out_packet
);
void InteractionSystem_ClearReleasedDrag(InteractionSystemState* state, const EngineInput* input);
void InteractionSystem_ApplyDragPacket(struct Scene* scene, const EngineRuntimeInputPacket* input_packet);

#endif
