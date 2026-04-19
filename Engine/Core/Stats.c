#include "Stats.h"

#include <string.h>
#include <time.h>

static double engine_stats_now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static void engine_stats_push_history(double* values, size_t* count, size_t capacity, double value) {
    if (values == 0 || count == 0 || capacity == 0) {
        return;
    }

    if (*count < capacity) {
        values[*count] = value;
        *count += 1;
        return;
    }

    memmove(values, values + 1, sizeof(double) * (capacity - 1));
    values[capacity - 1] = value;
}

void EngineStats_init(EngineStats* stats) {
    if (stats == 0) {
        return;
    }

    memset(stats, 0, sizeof(*stats));
    stats->history_capacity = ENGINE_STATS_HISTORY_CAPACITY;
    stats->fps_window_started_ms = engine_stats_now_ms();
    stats->last_frame_ended_ms = stats->fps_window_started_ms;
}

void EngineStats_begin_update(EngineStats* stats) {
    if (stats == 0) {
        return;
    }

    stats->update_started_ms = engine_stats_now_ms();
}

void EngineStats_end_update(EngineStats* stats) {
    if (stats == 0) {
        return;
    }

    stats->last_update_ms = engine_stats_now_ms() - stats->update_started_ms;
    if (stats->last_update_ms < 0.0) {
        stats->last_update_ms = 0.0;
    }
    engine_stats_push_history(stats->update_history_ms, &stats->history_count, stats->history_capacity, stats->last_update_ms);
}

void EngineStats_begin_draw(EngineStats* stats) {
    if (stats == 0) {
        return;
    }

    stats->draw_started_ms = engine_stats_now_ms();
}

void EngineStats_end_draw(EngineStats* stats) {
    double ended_ms = 0.0;
    double elapsed_window = 0.0;
    size_t frame_history_count = 0;

    if (stats == 0) {
        return;
    }

    ended_ms = engine_stats_now_ms();
    stats->last_draw_ms = ended_ms - stats->draw_started_ms;
    if (stats->last_draw_ms < 0.0) {
        stats->last_draw_ms = 0.0;
    }

    stats->frame_count += 1;
    stats->fps_window_frames += 1;
    stats->last_frame_ms = ended_ms - stats->last_frame_ended_ms;
    if (stats->last_frame_ms < 0.0) {
        stats->last_frame_ms = 0.0;
    }
    stats->last_frame_ended_ms = ended_ms;
    if (stats->last_frame_ms > 0.0) {
        stats->instant_fps = 1000.0 / stats->last_frame_ms;
    }

    frame_history_count = stats->history_count;
    engine_stats_push_history(stats->draw_history_ms, &frame_history_count, stats->history_capacity, stats->last_draw_ms);
    engine_stats_push_history(stats->frame_history_ms, &frame_history_count, stats->history_capacity, stats->last_frame_ms);

    elapsed_window = ended_ms - stats->fps_window_started_ms;
    if (elapsed_window >= 1000.0) {
        stats->fps = (double)stats->fps_window_frames * 1000.0 / elapsed_window;
        stats->fps_window_started_ms = ended_ms;
        stats->fps_window_frames = 0;
    }
}

EngineStatsSnapshot EngineStats_get_snapshot(const EngineStats* stats) {
    EngineStatsSnapshot snapshot;

    memset(&snapshot, 0, sizeof(snapshot));
    if (stats == 0) {
        return snapshot;
    }

    snapshot.update_ms = stats->last_update_ms;
    snapshot.draw_ms = stats->last_draw_ms;
    snapshot.frame_ms = stats->last_frame_ms;
    snapshot.instant_fps = stats->instant_fps;
    snapshot.fps = stats->fps;
    snapshot.frame_count = stats->frame_count;
    return snapshot;
}
