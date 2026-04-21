#ifndef JIMOKOMI_ENGINE_RUNTIMEINPUT_H
#define JIMOKOMI_ENGINE_RUNTIMEINPUT_H

#include "Core/Input.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct EngineRuntimeInputPacket
{
    EngineInputSnapshot snapshot;
    bool pointer_over_ui;
    bool debug_overlay_enabled;
    bool draw_debug_world;
    bool drag_entity_active;
    bool drag_entity_release;
    uint32_t drag_entity_id;
    float drag_world_x;
    float drag_world_y;
    float drag_linear_velocity_x;
    float drag_linear_velocity_y;
    float camera_x;
    float camera_y;
    float camera_view_width;
    float camera_view_height;
    int window_width;
    int window_height;
    uint64_t selected_entity_id;
    uint64_t hovered_entity_id;
    uint64_t frame_id;
} EngineRuntimeInputPacket;

#endif
