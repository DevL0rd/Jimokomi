#ifndef JIMOKOMI_GAME_INPUTPACKET_H
#define JIMOKOMI_GAME_INPUTPACKET_H

#include "../Engine/Core/Input.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct GameInputPacket {
    EngineInputSnapshot snapshot;
    bool pointer_over_ui;
    bool debug_overlay_enabled;
    bool draw_debug_world;
    bool drag_entity_active;
    bool drag_entity_release;
    size_t drag_ball_index;
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
    uint64_t frame_id;
} GameInputPacket;

#endif
