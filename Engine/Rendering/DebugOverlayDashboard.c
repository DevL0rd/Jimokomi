#include "DebugOverlayInternal.h"

#include <stdio.h>

typedef struct DebugMetricVisual {
    const char* label;
    float value;
    float budget;
    Color32 accent;
    const float* history;
} DebugMetricVisual;

typedef struct DebugStatChip {
    const char* label;
    const char* value_text;
    float ratio;
    Color32 accent;
} DebugStatChip;

static float debug_metric_ratio(float value, float budget) {
    if (budget <= 0.0f) {
        return 0.0f;
    }
    return clamp_f(value / budget, 0.0f, 1.0f);
}

static Color32 debug_metric_text_color(float value, float budget, Color32 accent) {
    if (budget > 0.0f && value > budget) {
        return (Color32){ 0xff8e7cU };
    }
    return accent;
}

static void debug_draw_history_graph(
    Target* target,
    Rect rect,
    const DebugOverlay* overlay,
    const DebugMetricVisual* metric
) {
    size_t sample_index;
    float graph_max = 33.3f;
    float previous_x = 0.0f;
    float previous_y = 0.0f;

    if (target == NULL || overlay == NULL || metric == NULL) {
        return;
    }

    target_rect_filled(target, rect, (Color32){ 0x0a1621U });
    target_rect(target, rect, (Color32){ 0x162838U });

    for (sample_index = 1U; sample_index <= 2U; ++sample_index) {
        float guide_ratio = (float)sample_index / 3.0f;
        float guide_y = rect.y + rect.h - rect.h * guide_ratio;
        target_line(target, rect.x, guide_y, rect.x + rect.w, guide_y, (Color32){ 0x132334U });
    }

    if (metric->budget > graph_max) {
        graph_max = metric->budget;
    }

    if (overlay->history_count < 2U || graph_max <= 0.0f) {
        return;
    }

    previous_x = rect.x;
    previous_y = rect.y + rect.h;

    for (sample_index = 0U; sample_index < overlay->history_count; ++sample_index) {
        float sample = debug_overlay_history_sample(metric->history, overlay->history_count, overlay->history_cursor, sample_index);
        float x = rect.x + ((float)sample_index / (float)(overlay->history_count - 1U)) * rect.w;
        float y = rect.y + rect.h - rect.h * clamp_f(sample / graph_max, 0.0f, 1.0f);

        if (sample_index > 0U) {
            target_line(target, previous_x, previous_y, x, y, metric->accent);
        }

        previous_x = x;
        previous_y = y;
    }
}

static void debug_draw_metric_card(
    Target* target,
    Rect rect,
    const DebugOverlay* overlay,
    const DebugMetricVisual* metric
) {
    Rect header_rect;
    Rect bar_rect;
    Rect graph_rect;
    Rect fill_rect;
    char text[48];
    float ratio;
    Color32 value_color;

    if (target == NULL || overlay == NULL || metric == NULL) {
        return;
    }

    ratio = debug_metric_ratio(metric->value, metric->budget);
    value_color = debug_metric_text_color(metric->value, metric->budget, metric->accent);
    header_rect = (Rect){ rect.x + 8.0f, rect.y + 6.0f, rect.w - 16.0f, 14.0f };
    bar_rect = (Rect){ rect.x + 8.0f, rect.y + 22.0f, rect.w - 16.0f, 2.0f };
    fill_rect = bar_rect;
    fill_rect.w *= ratio;
    graph_rect = (Rect){ rect.x + 8.0f, rect.y + 30.0f, rect.w - 16.0f, rect.h - 38.0f };

    target_rect_filled(target, rect, (Color32){ 0x0b1722U });
    target_rect(target, rect, (Color32){ 0x152737U });
    target_rect_filled(target, (Rect){ header_rect.x, header_rect.y + 3.0f, 4.0f, 4.0f }, metric->accent);
    target_text(target, header_rect.x + 8.0f, rect.y + 4.0f, metric->label, (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%.1f", metric->value);
    target_text(target, rect.x + rect.w - 38.0f, rect.y + 4.0f, text, value_color);
    target_rect_filled(target, bar_rect, (Color32){ 0x132333U });
    if (fill_rect.w > 0.0f) {
        target_rect_filled(target, fill_rect, metric->accent);
    }
    debug_draw_history_graph(target, graph_rect, overlay, metric);
}

static void debug_draw_stat_chip(
    Target* target,
    Rect rect,
    const DebugStatChip* chip
) {
    Rect label_rect;
    Rect value_rect;
    Rect bar_rect;
    Rect fill_rect;

    if (target == NULL || chip == NULL) {
        return;
    }

    label_rect = (Rect){ rect.x + 14.0f, rect.y + 3.0f, rect.w - 20.0f, 10.0f };
    value_rect = (Rect){ rect.x + 14.0f, rect.y + 15.0f, rect.w - 20.0f, 10.0f };
    bar_rect = (Rect){ rect.x + 6.0f, rect.y + rect.h - 5.0f, rect.w - 12.0f, 2.0f };
    fill_rect = bar_rect;
    fill_rect.w *= clamp_f(chip->ratio, 0.0f, 1.0f);

    target_rect_filled(target, rect, (Color32){ 0x0b1722U });
    target_rect(target, rect, (Color32){ 0x152737U });
    target_rect_filled(target, (Rect){ rect.x + 6.0f, rect.y + 6.0f, 4.0f, 4.0f }, chip->accent);
    target_text(target, label_rect.x, label_rect.y, chip->label, (Color32){ 0x7f99adU });
    target_text(target, value_rect.x, value_rect.y, chip->value_text, (Color32){ 0xdce7efU });
    target_rect_filled(target, bar_rect, (Color32){ 0x132333U });
    if (fill_rect.w > 0.0f) {
        target_rect_filled(target, fill_rect, chip->accent);
    }
}

void debug_overlay_draw_dashboard_contents(
    DebugOverlay* overlay,
    Target* target,
    const DebugOverlaySnapshot* snapshot,
    const EngineStatsSnapshot* stats,
    int viewport_width,
    int viewport_height
) {
    float left;
    float top;
    DebugMetricVisual metrics[7];
    Rect card_rect;
    Rect stat_rect;
    float column_width;
    float row_height;
    float stat_width;
    float stats_top;
    DebugStatChip stat_chips[11];
    char stat_text[11][24];

    if (overlay == NULL || target == NULL || snapshot == NULL || stats == NULL) {
        return;
    }

    (void)viewport_width;
    (void)viewport_height;

    metrics[0] = (DebugMetricVisual){ "FPS", overlay->display_fps, 120.0f, (Color32){ 0xb6f27cU }, overlay->fps_history };
    metrics[1] = (DebugMetricVisual){ "Render", overlay->display_render_ms, 16.67f, (Color32){ 0x78e6ffU }, overlay->draw_ms_history };
    metrics[2] = (DebugMetricVisual){ "1% Low", overlay->display_one_percent_low_fps, 120.0f, (Color32){ 0xffd07aU }, overlay->fps_history };
    metrics[3] = (DebugMetricVisual){ "Sim", overlay->display_sim_ms, 16.67f, (Color32){ 0x8aa8bcU }, overlay->sim_ms_history };
    metrics[4] = (DebugMetricVisual){ "Update", overlay->display_update_ms, 16.67f, (Color32){ 0x8fe8c4U }, overlay->update_ms_history };
    metrics[5] = (DebugMetricVisual){ "Physics", overlay->display_physics_ms, 16.67f, (Color32){ 0xff8d7aU }, overlay->physics_ms_history };
    metrics[6] = (DebugMetricVisual){ "Entities", overlay->display_visible_count, 20000.0f, (Color32){ 0xb896ffU }, overlay->visible_count_history };

    debug_draw_panel_frame(target, &overlay->dashboard_panel, "Performance", overlay->hovered_ui_region == DEBUG_UI_HOVER_DASHBOARD);
    left = overlay->dashboard_panel.x + 12.0f;
    top = overlay->dashboard_panel.y + 30.0f;
    column_width = (overlay->dashboard_panel.width - 36.0f) * 0.5f;
    row_height = 68.0f;

    card_rect = (Rect){ left, top, overlay->dashboard_panel.width - 24.0f, row_height };
    debug_draw_metric_card(target, card_rect, overlay, &metrics[0]);
    top += row_height + 10.0f;

    card_rect = (Rect){ left, top, column_width, row_height };
    debug_draw_metric_card(target, card_rect, overlay, &metrics[1]);
    card_rect.x += column_width + 12.0f;
    debug_draw_metric_card(target, card_rect, overlay, &metrics[2]);
    card_rect = (Rect){ left, top + (row_height + 10.0f), overlay->dashboard_panel.width - 24.0f, row_height };
    debug_draw_metric_card(target, card_rect, overlay, &metrics[3]);

    card_rect = (Rect){ left, card_rect.y + row_height + 10.0f, column_width, row_height };
    debug_draw_metric_card(target, card_rect, overlay, &metrics[4]);
    card_rect.x += column_width + 12.0f;
    debug_draw_metric_card(target, card_rect, overlay, &metrics[5]);

    card_rect = (Rect){ left, card_rect.y + row_height + 10.0f, overlay->dashboard_panel.width - 24.0f, row_height };
    debug_draw_metric_card(target, card_rect, overlay, &metrics[6]);

    snprintf(stat_text[0], sizeof(stat_text[0]), "%.1f ms", overlay->display_pairs_ms);
    snprintf(stat_text[1], sizeof(stat_text[1]), "%.1f ms", overlay->display_collide_ms);
    snprintf(stat_text[2], sizeof(stat_text[2]), "%.1f ms", overlay->display_solve_ms);
    snprintf(stat_text[3], sizeof(stat_text[3]), "%.0f", overlay->display_physics_hz);
    snprintf(stat_text[4], sizeof(stat_text[4]), "%.0f", overlay->display_awake_body_count);
    snprintf(stat_text[5], sizeof(stat_text[5]), "%.0f", overlay->display_total_body_count);
    snprintf(stat_text[6], sizeof(stat_text[6]), "%.0f", overlay->display_sleeping_body_count);
    snprintf(stat_text[7], sizeof(stat_text[7]), "%.0f", overlay->display_moved_body_count);
    snprintf(stat_text[8], sizeof(stat_text[8]), "%.0f", overlay->display_spatial_dirty_cells);
    snprintf(stat_text[9], sizeof(stat_text[9]), "%.0f", overlay->display_spatial_dirty_entities);
    snprintf(stat_text[10], sizeof(stat_text[10]), "%.1f ms", overlay->display_snapshot_age_ms);
    stat_chips[0] = (DebugStatChip){ "Pairs", stat_text[0], clamp_f(overlay->display_pairs_ms / 16.67f, 0.0f, 1.0f), (Color32){ 0x7fd7ffU } };
    stat_chips[1] = (DebugStatChip){ "Collide", stat_text[1], clamp_f(overlay->display_collide_ms / 16.67f, 0.0f, 1.0f), (Color32){ 0x96f0b7U } };
    stat_chips[2] = (DebugStatChip){ "Solve", stat_text[2], clamp_f(overlay->display_solve_ms / 16.67f, 0.0f, 1.0f), (Color32){ 0xff9a7aU } };
    stat_chips[3] = (DebugStatChip){ "Hz", stat_text[3], clamp_f(overlay->display_physics_hz / 120.0f, 0.0f, 1.0f), (Color32){ 0xb896ffU } };
    stat_chips[4] = (DebugStatChip){ "Awake", stat_text[4], clamp_f(overlay->display_awake_body_count / 20000.0f, 0.0f, 1.0f), (Color32){ 0xffd07aU } };
    stat_chips[5] = (DebugStatChip){ "Bodies", stat_text[5], clamp_f(overlay->display_total_body_count / 20000.0f, 0.0f, 1.0f), (Color32){ 0x8fe8c4U } };
    stat_chips[6] = (DebugStatChip){ "Sleep", stat_text[6], clamp_f(overlay->display_sleeping_body_count / 20000.0f, 0.0f, 1.0f), (Color32){ 0x7f99adU } };
    stat_chips[7] = (DebugStatChip){ "Moved", stat_text[7], clamp_f(overlay->display_moved_body_count / 20000.0f, 0.0f, 1.0f), (Color32){ 0x7ce2ffU } };
    stat_chips[8] = (DebugStatChip){ "Cells", stat_text[8], clamp_f(overlay->display_spatial_dirty_cells / 512.0f, 0.0f, 1.0f), (Color32){ 0xb896ffU } };
    stat_chips[9] = (DebugStatChip){ "Dirty", stat_text[9], clamp_f(overlay->display_spatial_dirty_entities / 20000.0f, 0.0f, 1.0f), (Color32){ 0xffb36bU } };
    stat_chips[10] = (DebugStatChip){ "Age", stat_text[10], clamp_f(overlay->display_snapshot_age_ms / 16.67f, 0.0f, 1.0f), (Color32){ 0xd8d08fU } };

    stat_width = (overlay->dashboard_panel.width - 24.0f - 10.0f) / 3.0f;
    stats_top = card_rect.y + row_height + 12.0f;
    stat_rect = (Rect){ left, stats_top, stat_width, 28.0f };
    debug_draw_stat_chip(target, stat_rect, &stat_chips[0]);
    stat_rect.x += stat_width + 5.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[1]);
    stat_rect.x += stat_width + 5.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[2]);
    stat_rect.x = left;
    stat_rect.y += 30.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[3]);
    stat_rect.x += stat_width + 5.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[4]);
    stat_rect.x += stat_width + 5.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[5]);
    stat_rect.x = left;
    stat_rect.y += 30.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[6]);
    stat_rect.x += stat_width + 5.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[7]);
    stat_rect.x += stat_width + 5.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[8]);
    stat_rect.x = left;
    stat_rect.y += 30.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[9]);
    stat_rect.x += stat_width + 5.0f;
    debug_draw_stat_chip(target, stat_rect, &stat_chips[10]);
}
