#ifndef JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_HISTORY_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_HISTORY_INTERNAL_H

#include "DebugOverlayInternal.h"

struct DebugOverlayHistoryState {
    uint64_t history_serial;
    uint64_t last_history_push_ms;
    float last_one_percent_low_fps;
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
    float display_physics_hz;
    float display_awake_body_count;
    float display_total_body_count;
    float display_sleeping_body_count;
    float display_moved_body_count;
    float display_snapshot_age_ms;
    size_t history_count;
    size_t history_cursor;
};

#endif
