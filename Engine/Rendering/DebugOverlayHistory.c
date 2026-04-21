#include "DebugOverlayInternal.h"

static float debug_smooth_display_value(float current, float target) {
    if (current <= 0.0f) {
        return target;
    }
    return current + ((target - current) * 0.18f);
}

float debug_overlay_history_sample(const float* history, size_t history_count, size_t history_cursor, size_t index) {
    size_t start;

    if (history == NULL || history_count == 0U || index >= history_count) {
        return 0.0f;
    }

    start = history_count == DEBUG_OVERLAY_HISTORY_CAPACITY ? history_cursor : 0U;
    return history[(start + index) % DEBUG_OVERLAY_HISTORY_CAPACITY];
}

void debug_overlay_history_push(DebugOverlay* overlay, const DebugOverlaySnapshot* snapshot, const EngineStatsSnapshot* stats) {
    size_t slot;

    if (overlay == NULL || snapshot == NULL || stats == NULL) {
        return;
    }

    slot = overlay->history_cursor;
    overlay->fps_history[slot] = snapshot->fps;
    overlay->frame_ms_history[slot] = (float)stats->frame_ms;
    overlay->update_ms_history[slot] = snapshot->update_ms;
    overlay->sim_ms_history[slot] = snapshot->sim_ms;
    overlay->draw_ms_history[slot] = snapshot->draw_ms;
    overlay->physics_ms_history[slot] = snapshot->physics_ms;
    overlay->visible_count_history[slot] = (float)snapshot->visible_count;
    overlay->history_cursor = (slot + 1U) % DEBUG_OVERLAY_HISTORY_CAPACITY;
    if (overlay->history_count < DEBUG_OVERLAY_HISTORY_CAPACITY) {
        overlay->history_count += 1U;
    }
    overlay->history_serial += 1U;
}

float debug_overlay_compute_one_percent_low_fps(const DebugOverlay* overlay) {
    float worst_samples[DEBUG_OVERLAY_HISTORY_CAPACITY];
    size_t count;
    size_t bucket_count;
    size_t i;
    size_t j;
    float accumulated_frame_ms = 0.0f;

    if (overlay == NULL || overlay->history_count == 0U) {
        return 0.0f;
    }

    count = overlay->history_count;
    bucket_count = (count + 99U) / 100U;
    if (bucket_count == 0U) {
        bucket_count = 1U;
    }
    if (bucket_count > count) {
        bucket_count = count;
    }

    for (i = 0U; i < bucket_count; ++i) {
        worst_samples[i] = 0.0f;
    }

    for (i = 0U; i < bucket_count; ++i) {
        worst_samples[i] = debug_overlay_history_sample(
            overlay->frame_ms_history,
            overlay->history_count,
            overlay->history_cursor,
            i
        );
    }
    for (i = bucket_count; i < count; ++i) {
        float sample = debug_overlay_history_sample(
            overlay->frame_ms_history,
            overlay->history_count,
            overlay->history_cursor,
            i
        );

        if (sample <= worst_samples[bucket_count - 1U]) {
            continue;
        }

        worst_samples[bucket_count - 1U] = sample;
        for (j = bucket_count - 1U; j > 0U && worst_samples[j] > worst_samples[j - 1U]; --j) {
            float temp = worst_samples[j];
            worst_samples[j] = worst_samples[j - 1U];
            worst_samples[j - 1U] = temp;
        }
    }

    for (i = 0U; i < bucket_count; ++i) {
        accumulated_frame_ms += worst_samples[i];
    }

    accumulated_frame_ms /= (float)bucket_count;
    if (accumulated_frame_ms <= 0.0f) {
        return 0.0f;
    }

    return 1000.0f / accumulated_frame_ms;
}

void debug_overlay_update_display_values(
    DebugOverlay* overlay,
    const DebugOverlaySnapshot* snapshot,
    float one_percent_low_fps
) {
    if (overlay == NULL || snapshot == NULL) {
        return;
    }

    overlay->display_fps = debug_smooth_display_value(overlay->display_fps, snapshot->fps);
    overlay->display_render_ms = debug_smooth_display_value(overlay->display_render_ms, snapshot->draw_ms);
    overlay->display_one_percent_low_fps =
        debug_smooth_display_value(overlay->display_one_percent_low_fps, one_percent_low_fps);
    overlay->display_update_ms = debug_smooth_display_value(overlay->display_update_ms, snapshot->update_ms);
    overlay->display_sim_ms = debug_smooth_display_value(overlay->display_sim_ms, snapshot->sim_ms);
    overlay->display_physics_ms = debug_smooth_display_value(overlay->display_physics_ms, snapshot->physics_ms);
    overlay->display_visible_count =
        debug_smooth_display_value(overlay->display_visible_count, (float)snapshot->visible_count);
    overlay->display_physics_hz =
        debug_smooth_display_value(overlay->display_physics_hz, snapshot->physics_hz);
    overlay->display_awake_body_count =
        debug_smooth_display_value(overlay->display_awake_body_count, (float)snapshot->awake_body_count);
    overlay->display_total_body_count =
        debug_smooth_display_value(overlay->display_total_body_count, (float)snapshot->total_body_count);
    overlay->display_sleeping_body_count =
        debug_smooth_display_value(overlay->display_sleeping_body_count, (float)snapshot->sleeping_body_count);
    overlay->display_moved_body_count =
        debug_smooth_display_value(overlay->display_moved_body_count, (float)snapshot->moved_body_count);
    overlay->display_snapshot_age_ms =
        debug_smooth_display_value(overlay->display_snapshot_age_ms, snapshot->snapshot_age_ms);
}
