#ifndef JIMOKOMI_ENGINE_CORE_STATS_H
#define JIMOKOMI_ENGINE_CORE_STATS_H

#include <stddef.h>

#define ENGINE_STATS_HISTORY_CAPACITY 90

typedef struct EngineStatsSnapshot {
    double update_ms;
    double draw_ms;
    double frame_ms;
    double instant_fps;
    double fps;
    size_t frame_count;
} EngineStatsSnapshot;

typedef struct EngineStats {
    double last_update_ms;
    double last_draw_ms;
    double last_frame_ms;
    double instant_fps;
    size_t frame_count;
    double fps;

    size_t history_capacity;
    double update_history_ms[ENGINE_STATS_HISTORY_CAPACITY];
    double draw_history_ms[ENGINE_STATS_HISTORY_CAPACITY];
    double frame_history_ms[ENGINE_STATS_HISTORY_CAPACITY];
    size_t history_count;

    double fps_window_started_ms;
    size_t fps_window_frames;
    double update_started_ms;
    double draw_started_ms;
    double last_frame_ended_ms;
} EngineStats;

void EngineStats_init(EngineStats* stats);
void EngineStats_begin_update(EngineStats* stats);
void EngineStats_end_update(EngineStats* stats);
void EngineStats_begin_draw(EngineStats* stats);
void EngineStats_end_draw(EngineStats* stats);
EngineStatsSnapshot EngineStats_get_snapshot(const EngineStats* stats);

#endif
