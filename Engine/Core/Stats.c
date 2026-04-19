#include "Stats.h"

#include <string.h>
#include <time.h>

static double engine_stats_now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

void EngineStats_init(EngineStats* stats) {
    if (stats == 0) {
        return;
    }

    memset(stats, 0, sizeof(*stats));
    stats->last_frame_ended_ms = engine_stats_now_ms();
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
}

void EngineStats_begin_draw(EngineStats* stats) {
    if (stats == 0) {
        return;
    }

    stats->draw_started_ms = engine_stats_now_ms();
}

void EngineStats_end_draw(EngineStats* stats) {
    double ended_ms = 0.0;

    if (stats == 0) {
        return;
    }

    ended_ms = engine_stats_now_ms();
    stats->last_draw_ms = ended_ms - stats->draw_started_ms;
    if (stats->last_draw_ms < 0.0) {
        stats->last_draw_ms = 0.0;
    }

    stats->frame_count += 1;
    stats->last_frame_ms = ended_ms - stats->last_frame_ended_ms;
    if (stats->last_frame_ms < 0.0) {
        stats->last_frame_ms = 0.0;
    }
    stats->last_frame_ended_ms = ended_ms;
    if (stats->last_frame_ms > 0.0) {
        stats->instant_fps = 1000.0 / stats->last_frame_ms;
        if (stats->fps <= 0.0) {
            stats->fps = stats->instant_fps;
        } else {
            stats->fps = stats->fps * 0.9 + stats->instant_fps * 0.1;
        }
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
