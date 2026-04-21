#include "DebugOverlayInternal.h"

#include "../Settings.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum {
    DEBUG_UI_HOVER_NONE = 0,
    DEBUG_UI_HOVER_DASHBOARD = 1,
    DEBUG_UI_HOVER_INSPECTOR = 2
};

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

static uint64_t debug_overlay_compute_world_signature(
    const DebugOverlay* overlay,
    const Camera* camera,
    const DebugGridView* grid,
    const DebugEntityView* entities,
    size_t entity_count,
    const DebugCollisionView* collisions,
    size_t collision_count,
    uint64_t selected_entity_id,
    uint64_t hovered_entity_id
);

static double debug_overlay_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static float debug_title_bar_height(void) {
    return EngineSettings_GetDefaults()->debug_overlay_title_bar_height;
}

static float debug_inspector_collapsed_width(void) {
    return EngineSettings_GetDefaults()->debug_overlay_inspector_collapsed_width;
}

static uint64_t debug_history_push_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_history_push_interval_ms;
}

static uint64_t debug_dashboard_redraw_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_dashboard_redraw_interval_ms;
}

static uint64_t debug_inspector_redraw_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_inspector_redraw_interval_ms;
}

static uint64_t debug_hash_u64(uint64_t hash, uint64_t value) {
    hash ^= value;
    hash *= 1099511628211ULL;
    return hash;
}

static uint64_t debug_hash_i32(uint64_t hash, int32_t value) {
    return debug_hash_u64(hash, (uint32_t)value);
}

static int32_t debug_round_tenths(float value) {
    return (int32_t)lrintf(value * 10.0f);
}

static int32_t debug_round_whole(float value) {
    return (int32_t)lrintf(value);
}

static int32_t debug_round_quarters(float value) {
    return (int32_t)lrintf(value * 4.0f);
}

static bool debug_point_in_rect(float x, float y, Rect rect) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

static Rect debug_panel_rect(const DebugPanelState *panel) {
    Rect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (panel != NULL) {
        rect.x = panel->x;
        rect.y = panel->y;
        rect.w = panel->width;
        rect.h = panel->height;
    }
    return rect;
}

static Rect debug_title_rect(const DebugPanelState *panel) {
    Rect rect = debug_panel_rect(panel);
    rect.h = debug_title_bar_height();
    return rect;
}

static Rect debug_inspector_collapse_rect(const DebugPanelState* panel) {
    Rect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (panel != NULL) {
        rect.x = panel->x;
        rect.y = panel->y;
        rect.w = debug_inspector_collapsed_width();
        rect.h = panel->height;
    }
    return rect;
}

static void debug_draw_panel_frame(
    Target *target,
    const DebugPanelState *panel,
    const char *title,
    bool hovered
) {
    Rect rect = debug_panel_rect(panel);
    Rect title_rect = debug_title_rect(panel);
    Color32 title_fill = hovered ? (Color32){ 0x132435U } : (Color32){ 0x0c1823U };

    target_rect_filled(target, rect, (Color32){ 0x08111aU });
    target_rect(target, rect, (Color32){ 0x22364aU });
    target_rect_filled(target, title_rect, title_fill);
    target_line(target, rect.x, rect.y + title_rect.h, rect.x + rect.w, rect.y + title_rect.h, (Color32){ 0x1a2b3aU });
    target_text(target, rect.x + 10.0f, rect.y + 4.0f, title != NULL ? title : "Panel", (Color32){ 0xf1f7fbU });
}

static void debug_draw_inspector_rail(Target* target, const DebugPanelState* panel, bool hovered) {
    Rect rail_rect = debug_inspector_collapse_rect(panel);
    Color32 title_fill = hovered ? (Color32){ 0x132435U } : (Color32){ 0x0c1823U };

    if (target == NULL || panel == NULL) {
        return;
    }

    target_rect_filled(target, rail_rect, (Color32){ 0x08111aU });
    target_rect(target, rail_rect, (Color32){ 0x22364aU });
    target_rect_filled(target, (Rect){ rail_rect.x, rail_rect.y, rail_rect.w, debug_title_bar_height() }, title_fill);
    target_line(target, rail_rect.x, rail_rect.y + debug_title_bar_height(), rail_rect.x + rail_rect.w, rail_rect.y + debug_title_bar_height(), (Color32){ 0x1a2b3aU });
    target_text(target, rail_rect.x + 7.0f, rail_rect.y + 32.0f, "I", (Color32){ 0xf1f7fbU });
    target_text(target, rail_rect.x + 7.0f, rail_rect.y + 52.0f, "N", (Color32){ 0x9ab0c1U });
    target_text(target, rail_rect.x + 7.0f, rail_rect.y + 72.0f, "F", (Color32){ 0x9ab0c1U });
    target_text(target, rail_rect.x + 7.0f, rail_rect.y + 92.0f, "O", (Color32){ 0x9ab0c1U });
}

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

static float debug_smooth_display_value(float current, float target) {
    if (current <= 0.0f) {
        return target;
    }
    return current + ((target - current) * 0.18f);
}

static void debug_history_push(DebugOverlay* overlay, const DebugOverlaySnapshot* snapshot, const EngineStatsSnapshot* stats) {
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

static float debug_history_sample(const float* history, size_t history_count, size_t history_cursor, size_t index) {
    size_t start;

    if (history == NULL || history_count == 0U || index >= history_count) {
        return 0.0f;
    }

    start = history_count == DEBUG_OVERLAY_HISTORY_CAPACITY ? history_cursor : 0U;
    return history[(start + index) % DEBUG_OVERLAY_HISTORY_CAPACITY];
}

static float debug_compute_one_percent_low_fps(const DebugOverlay* overlay) {
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
        worst_samples[i] = debug_history_sample(
            overlay->frame_ms_history,
            overlay->history_count,
            overlay->history_cursor,
            i
        );
    }
    for (i = bucket_count; i < count; ++i) {
        float sample = debug_history_sample(
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

static void debug_update_display_values(
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
    overlay->display_pairs_ms =
        debug_smooth_display_value(overlay->display_pairs_ms, snapshot->physics_pairs_ms);
    overlay->display_collide_ms =
        debug_smooth_display_value(overlay->display_collide_ms, snapshot->physics_collide_ms);
    overlay->display_solve_ms =
        debug_smooth_display_value(overlay->display_solve_ms, snapshot->physics_solve_ms);
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
    overlay->display_spatial_dirty_cells =
        debug_smooth_display_value(overlay->display_spatial_dirty_cells, (float)snapshot->spatial_dirty_cells);
    overlay->display_spatial_dirty_entities =
        debug_smooth_display_value(overlay->display_spatial_dirty_entities, (float)snapshot->spatial_dirty_entities);
    overlay->display_snapshot_age_ms =
        debug_smooth_display_value(overlay->display_snapshot_age_ms, snapshot->snapshot_age_ms);
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
        float sample = debug_history_sample(metric->history, overlay->history_count, overlay->history_cursor, sample_index);
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

static void debug_overlay_ensure_layout(
    DebugOverlay *overlay,
    bool has_selection,
    int viewport_width,
    int viewport_height
) {
    if (overlay == NULL) {
        return;
    }

    if (overlay->layout_initialized &&
        overlay->last_layout_viewport_width == viewport_width &&
        overlay->last_layout_viewport_height == viewport_height &&
        overlay->last_layout_has_selection == has_selection) {
        return;
    }

    if (!overlay->layout_initialized) {
        overlay->dashboard_panel.width = 336.0f;
        overlay->dashboard_panel.height = 548.0f;
        overlay->dashboard_panel.x = 12.0f;
        overlay->dashboard_panel.y = 12.0f;

        overlay->inspector_panel.width = 336.0f;
        overlay->inspector_panel.height = 166.0f;
        overlay->inspector_panel.x = (float)viewport_width - debug_inspector_collapsed_width() - 12.0f;
        overlay->inspector_panel.y = 12.0f;
        overlay->inspector_collapsed = true;
        overlay->layout_initialized = true;
    }

    overlay->dashboard_panel.x = clamp_f(overlay->dashboard_panel.x, 0.0f, (float)viewport_width - overlay->dashboard_panel.width);
    overlay->dashboard_panel.y = clamp_f(overlay->dashboard_panel.y, 0.0f, (float)viewport_height - overlay->dashboard_panel.height);
    overlay->inspector_panel.x = clamp_f(
        overlay->inspector_panel.x,
        0.0f,
        (float)viewport_width - (overlay->inspector_collapsed ? debug_inspector_collapsed_width() : overlay->inspector_panel.width)
    );
    overlay->inspector_panel.y = clamp_f(overlay->inspector_panel.y, 0.0f, (float)viewport_height - overlay->inspector_panel.height);

    if (!has_selection) {
        overlay->inspector_panel.dragging = false;
        overlay->inspector_collapsed = true;
    }

    overlay->last_layout_viewport_width = viewport_width;
    overlay->last_layout_viewport_height = viewport_height;
    overlay->last_layout_has_selection = has_selection;
}

static bool debug_handle_panel_drag(
    DebugPanelState *panel,
    const EngineInput *input,
    bool pressed,
    bool released
) {
    Rect title_rect;
    if (panel == NULL || input == NULL) {
        return false;
    }

    title_rect = debug_title_rect(panel);
    if (pressed && debug_point_in_rect((float)input->mouse_x, (float)input->mouse_y, title_rect)) {
        panel->dragging = true;
        panel->drag_offset_x = (float)input->mouse_x - panel->x;
        panel->drag_offset_y = (float)input->mouse_y - panel->y;
        return true;
    }

    if (panel->dragging) {
        if (released || !EngineInput_is_mouse_down(input, 1U)) {
            panel->dragging = false;
        } else {
            panel->x = (float)input->mouse_x - panel->drag_offset_x;
            panel->y = (float)input->mouse_y - panel->drag_offset_y;
        }
        return true;
    }

    return false;
}

static void debug_draw_entity(
    Target *target,
    const Camera *camera,
    const DebugEntityView *entity,
    bool is_selected,
    bool is_hovered
) {
    Vec2 screen_position;
    Vec2 screen_extent;
    Vec2 heading;
    Vec2 right;
    Vec2 velocity;
    float speed;
    float velocity_scale;
    Color32 outline = { 0x49ffaeU };
    Color32 heading_color = { 0x7ee0ffU };
    Color32 velocity_color = { 0xffd166U };

    if (target == NULL || camera == NULL || entity == NULL) {
        return;
    }

    if (is_selected) {
        outline.value = 0x00ffffU;
    } else if (is_hovered) {
        outline.value = 0x8de6ffU;
    } else if (entity->is_sleeping) {
        outline.value = 0x6b7890U;
    }

    screen_position = camera_world_to_screen(camera, entity->position);
    screen_extent = camera_world_size_to_screen(
        camera,
        (Vec2){
            entity->is_circle ? entity->radius : entity->extent_x,
            entity->is_circle ? entity->radius : entity->extent_y
        }
    );
    if (entity->is_circle) {
        target_circle(target, screen_position, screen_extent.x, outline);
    } else {
        target_rect(target, (Rect){
            screen_position.x - screen_extent.x,
            screen_position.y - screen_extent.y,
            screen_extent.x * 2.0f,
            screen_extent.y * 2.0f
        }, outline);
    }

    heading.x = cosf(entity->angle_radians);
    heading.y = sinf(entity->angle_radians);
    right.x = -heading.y;
    right.y = heading.x;
    target_line(
        target,
        screen_position.x,
        screen_position.y,
        screen_position.x + heading.x * 16.0f,
        screen_position.y + heading.y * 16.0f,
        heading_color
    );
    target_line(
        target,
        screen_position.x - right.x * 6.0f,
        screen_position.y - right.y * 6.0f,
        screen_position.x + right.x * 6.0f,
        screen_position.y + right.y * 6.0f,
        heading_color
    );

    speed = sqrtf(entity->velocity.x * entity->velocity.x + entity->velocity.y * entity->velocity.y);
    velocity_scale = clamp_f(speed * 0.55f, 0.0f, 20.0f);
    velocity.x = speed > 0.001f ? (entity->velocity.x / speed) * velocity_scale : 0.0f;
    velocity.y = speed > 0.001f ? (entity->velocity.y / speed) * velocity_scale : 0.0f;
    target_line(
        target,
        screen_position.x,
        screen_position.y,
        screen_position.x + velocity.x,
        screen_position.y + velocity.y,
        velocity_color
    );

}

static bool debug_entity_visible(const Camera* camera, const DebugEntityView* entity) {
    float extent_x;
    float extent_y;

    if (camera == NULL || entity == NULL) {
        return false;
    }

    extent_x = entity->is_circle ? entity->radius : entity->extent_x;
    extent_y = entity->is_circle ? entity->radius : entity->extent_y;

    return entity->position.x + extent_x >= camera->x &&
           entity->position.y + extent_y >= camera->y &&
           entity->position.x - extent_x <= camera->x + camera->view_width &&
           entity->position.y - extent_y <= camera->y + camera->view_height;
}

static void debug_draw_spatial_grid(
    Target* target,
    const Camera* camera,
    const DebugGridView* grid
) {
    int min_cell_x;
    int max_cell_x;
    int min_cell_y;
    int max_cell_y;
    int cell_x;
    int cell_y;

    if (target == NULL || camera == NULL || grid == NULL || !grid->enabled || grid->cell_size <= 0.0f) {
        return;
    }

    min_cell_x = clamp_i((int)floorf((camera->x - grid->min_x) / grid->cell_size), 0, grid->span_max_x);
    max_cell_x = clamp_i((int)ceilf((camera->x + camera->view_width - grid->min_x) / grid->cell_size), 0, grid->span_max_x);
    min_cell_y = clamp_i((int)floorf((camera->y - grid->min_y) / grid->cell_size), 0, grid->span_max_y);
    max_cell_y = clamp_i((int)ceilf((camera->y + camera->view_height - grid->min_y) / grid->cell_size), 0, grid->span_max_y);

    for (cell_x = min_cell_x; cell_x <= max_cell_x + 1; ++cell_x) {
        float world_x = grid->min_x + (float)cell_x * grid->cell_size;
        Vec2 screen_a = camera_world_to_screen(camera, (Vec2){ world_x, camera->y });
        Vec2 screen_b = camera_world_to_screen(camera, (Vec2){ world_x, camera->y + camera->view_height });
        Color32 color = (Color32){ 0x17303cU };
        if (cell_x == grid->span_min_x || cell_x == grid->span_max_x + 1) {
            color = (Color32){ 0x4bb8ccU };
        }
        target_line(target, screen_a.x, screen_a.y, screen_b.x, screen_b.y, color);
    }

    for (cell_y = min_cell_y; cell_y <= max_cell_y + 1; ++cell_y) {
        float world_y = grid->min_y + (float)cell_y * grid->cell_size;
        Vec2 screen_a = camera_world_to_screen(camera, (Vec2){ camera->x, world_y });
        Vec2 screen_b = camera_world_to_screen(camera, (Vec2){ camera->x + camera->view_width, world_y });
        Color32 color = (Color32){ 0x17303cU };
        if (cell_y == grid->span_min_y || cell_y == grid->span_max_y + 1) {
            color = (Color32){ 0x4bb8ccU };
        }
        target_line(target, screen_a.x, screen_a.y, screen_b.x, screen_b.y, color);
    }
}

void debug_overlay_init(DebugOverlay *overlay) {
    if (overlay == NULL) {
        return;
    }
    memset(overlay, 0, sizeof(*overlay));
    overlay->draw_ui = true;
    overlay->draw_world_gizmos = false;
    overlay->draw_ui_bounds = true;
    overlay->last_visible_entity_count = 0U;
    overlay->last_active_collision_count = 0U;
}

void debug_overlay_dispose(DebugOverlay *overlay) {
    if (overlay == NULL) {
        return;
    }
    if (overlay->dashboard_surface != NULL &&
        overlay->dashboard_surface_backend != NULL &&
        overlay->dashboard_surface_backend->destroy_surface != NULL) {
        overlay->dashboard_surface_backend->destroy_surface(
            overlay->dashboard_surface_backend->userdata,
            overlay->dashboard_surface
        );
    }
    if (overlay->inspector_surface != NULL &&
        overlay->inspector_surface_backend != NULL &&
        overlay->inspector_surface_backend->destroy_surface != NULL) {
        overlay->inspector_surface_backend->destroy_surface(
            overlay->inspector_surface_backend->userdata,
            overlay->inspector_surface
        );
    }
    if (overlay->world_surface != NULL &&
        overlay->world_surface_backend != NULL &&
        overlay->world_surface_backend->destroy_surface != NULL) {
        overlay->world_surface_backend->destroy_surface(
            overlay->world_surface_backend->userdata,
            overlay->world_surface
        );
    }
    memset(overlay, 0, sizeof(*overlay));
}

void debug_overlay_handle_input(
    DebugOverlay *overlay,
    const EngineInput *input,
    bool has_selection,
    int viewport_width,
    int viewport_height
) {
    bool pressed;
    bool released;
    bool handled;

    if (overlay == NULL || input == NULL || !overlay->draw_ui) {
        return;
    }

    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    pressed = EngineInput_was_mouse_pressed(input, 1U);
    released = EngineInput_was_mouse_released(input, 1U);

    handled = debug_handle_panel_drag(&overlay->dashboard_panel, input, pressed, released);
    overlay->hovered_ui_region = DEBUG_UI_HOVER_NONE;
    if (debug_point_in_rect((float)input->mouse_x, (float)input->mouse_y, debug_title_rect(&overlay->dashboard_panel))) {
        overlay->hovered_ui_region = DEBUG_UI_HOVER_DASHBOARD;
    }
    if (!handled && has_selection) {
        if (debug_point_in_rect((float)input->mouse_x, (float)input->mouse_y, overlay->inspector_collapsed
                ? debug_inspector_collapse_rect(&overlay->inspector_panel)
                : debug_title_rect(&overlay->inspector_panel))) {
            overlay->hovered_ui_region = DEBUG_UI_HOVER_INSPECTOR;
        }
        if (pressed && debug_point_in_rect((float)input->mouse_x, (float)input->mouse_y, overlay->inspector_collapsed
                ? debug_inspector_collapse_rect(&overlay->inspector_panel)
                : debug_title_rect(&overlay->inspector_panel))) {
            overlay->inspector_collapsed = !overlay->inspector_collapsed;
            if (overlay->inspector_collapsed) {
                overlay->inspector_panel.x = (float)viewport_width - debug_inspector_collapsed_width() - 12.0f;
            } else {
                overlay->inspector_panel.x = (float)viewport_width - overlay->inspector_panel.width - 12.0f;
            }
            overlay->inspector_panel.dragging = false;
            handled = true;
        }
        if (!handled && !overlay->inspector_collapsed) {
            (void)debug_handle_panel_drag(&overlay->inspector_panel, input, pressed, released);
        }
    } else if (!has_selection) {
        overlay->inspector_panel.dragging = false;
    }
}

bool debug_overlay_is_pointer_over_ui(
    DebugOverlay *overlay,
    bool has_selection,
    float screen_x,
    float screen_y,
    int viewport_width,
    int viewport_height
) {
    if (overlay == NULL || !overlay->draw_ui) {
        return false;
    }

    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    if (debug_point_in_rect(screen_x, screen_y, debug_panel_rect(&overlay->dashboard_panel))) {
        return true;
    }
    if (has_selection && debug_point_in_rect(
            screen_x,
            screen_y,
            overlay->inspector_collapsed
                ? debug_inspector_collapse_rect(&overlay->inspector_panel)
                : debug_panel_rect(&overlay->inspector_panel))) {
        return true;
    }
    return false;
}

void debug_overlay_draw_world(
    DebugOverlay *overlay,
    RenderBackend *backend,
    uint64_t now_ms,
    const Camera *camera,
    const DebugGridView *grid,
    const DebugEntityView *entities,
    size_t entity_count,
    const DebugCollisionView *collisions,
    size_t collision_count,
    uint64_t selected_entity_id,
    uint64_t hovered_entity_id
) {
    size_t index;
    Target target;
    bool has_selected_entity;
    bool has_hovered_entity;
    uint64_t world_signature;
    double redraw_started_ms = 0.0;
    (void)now_ms;

    if (overlay == NULL || !overlay->draw_world_gizmos || backend == NULL || camera == NULL) {
        return;
    }

    if (backend->create_surface == NULL ||
        backend->destroy_surface == NULL ||
        backend->set_target == NULL ||
        backend->reset_target == NULL ||
        backend->clear == NULL) {
        return;
    }

    if (overlay->world_surface == NULL ||
        overlay->world_surface_width != (int)camera->viewport_width ||
        overlay->world_surface_height != (int)camera->viewport_height) {
        if (overlay->world_surface != NULL) {
            backend->destroy_surface(backend->userdata, overlay->world_surface);
        }
        overlay->world_surface = backend->create_surface(
            backend->userdata,
            (int)camera->viewport_width,
            (int)camera->viewport_height
        );
        overlay->world_surface_backend = backend;
        overlay->world_surface_width = (int)camera->viewport_width;
        overlay->world_surface_height = (int)camera->viewport_height;
    }

    if (overlay->world_surface == NULL) {
        return;
    }

    world_signature = debug_overlay_compute_world_signature(
        overlay,
        camera,
        grid,
        entities,
        entity_count,
        collisions,
        collision_count,
        selected_entity_id,
        hovered_entity_id
    );
    overlay->last_visible_entity_count = 0U;
    overlay->last_active_collision_count = 0U;
    has_selected_entity = selected_entity_id != 0U;
    has_hovered_entity = hovered_entity_id != 0U;

    if (overlay->last_world_signature != world_signature) {
        redraw_started_ms = debug_overlay_now_ms();
        backend->set_target(backend->userdata, overlay->world_surface);
        backend->clear(backend->userdata, (Color32){ 0x00000000U });
        target_init(&target, backend, 0.0f, 0.0f);

        debug_draw_spatial_grid(&target, camera, grid);

        for (index = 0U; index < entity_count; ++index) {
            if (!debug_entity_visible(camera, &entities[index])) {
                continue;
            }
            overlay->last_visible_entity_count += 1U;
            debug_draw_entity(
                &target,
                camera,
                &entities[index],
                has_selected_entity && entities[index].id == selected_entity_id,
                has_hovered_entity && entities[index].id == hovered_entity_id
            );
        }
        for (index = 0U; index < collision_count; ++index) {
            Vec2 a;
            Vec2 b;
            Vec2 c;
            Vec2 n;
            Color32 color;
            if (!collisions[index].active) {
                continue;
            }
            overlay->last_active_collision_count += 1U;
            a = camera_world_to_screen(camera, collisions[index].point_a);
            b = camera_world_to_screen(camera, collisions[index].point_b);
            c = camera_world_to_screen(camera, collisions[index].contact_point);
            n = collisions[index].contact_normal;
            color.value = collisions[index].sleeping ? 0x666666U : (collisions[index].sensor ? 0xff00ffU : 0xffff00U);
            target_line(&target, a.x, a.y, b.x, b.y, color);
            target_circle_filled(&target, c, camera_world_size_to_screen(camera, (Vec2){ 1.5f, 1.5f }).x, color);
            target_line(&target, c.x, c.y, c.x + n.x * 8.0f, c.y + n.y * 8.0f, color);
        }

        backend->reset_target(backend->userdata);
        overlay->last_world_signature = world_signature;
        overlay->world_redraw_count += 1U;
        overlay->last_world_redraw_ms = debug_overlay_now_ms() - redraw_started_ms;
    } else {
        overlay->world_redraw_skip_count += 1U;
    }

    target_init(&target, backend, 0.0f, 0.0f);
    target_surface(&target, overlay->world_surface, 0.0f, 0.0f);
}

static void debug_overlay_tick_dashboard_state(
    DebugOverlay* overlay,
    const DebugOverlaySnapshot* snapshot,
    const EngineStatsSnapshot* stats
) {
    bool pushed_history = false;
    uint64_t now_ms = 0U;

    if (overlay == NULL || snapshot == NULL || stats == NULL) {
        return;
    }

    now_ms = (uint64_t)debug_overlay_now_ms();
    if (overlay->last_history_push_ms == 0U || stats->frame_ms <= 0.0 ||
        overlay->last_history_push_ms + debug_history_push_interval_ms() <= now_ms) {
        debug_history_push(overlay, snapshot, stats);
        overlay->last_history_push_ms = now_ms;
        pushed_history = true;
    }

    if (pushed_history || overlay->last_one_percent_low_fps <= 0.0f) {
        overlay->last_one_percent_low_fps = debug_compute_one_percent_low_fps(overlay);
    }

    debug_update_display_values(overlay, snapshot, overlay->last_one_percent_low_fps);
}

static bool debug_overlay_should_redraw_panel(
    uint64_t last_signature,
    uint64_t next_signature,
    uint64_t last_redraw_at_ms,
    uint64_t now_ms,
    uint64_t min_redraw_interval_ms,
    bool is_interacting
) {
    if (last_signature == 0ULL || last_signature != next_signature) {
        if (last_signature == 0ULL || is_interacting) {
            return true;
        }
        return last_redraw_at_ms == 0U ||
            now_ms >= last_redraw_at_ms + min_redraw_interval_ms;
    }

    return false;
}

static uint64_t debug_overlay_compute_dashboard_signature(
    const DebugOverlay* overlay,
    int viewport_width,
    int viewport_height
) {
    uint64_t hash = 1469598103934665603ULL;

    if (overlay == NULL) {
        return 0ULL;
    }

    hash = debug_hash_i32(hash, viewport_width);
    hash = debug_hash_i32(hash, viewport_height);
    hash = debug_hash_i32(hash, debug_round_whole(overlay->display_fps));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->display_render_ms));
    hash = debug_hash_i32(hash, debug_round_whole(overlay->display_one_percent_low_fps));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->display_sim_ms));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->display_update_ms));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->display_physics_ms));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->display_pairs_ms));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->display_collide_ms));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->display_solve_ms));
    hash = debug_hash_i32(hash, debug_round_whole(overlay->display_physics_hz));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_visible_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_awake_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_total_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_sleeping_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_moved_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)overlay->display_spatial_dirty_cells);
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_spatial_dirty_entities / 16.0f));
    hash = debug_hash_u64(hash, overlay->history_serial);
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->dashboard_panel.x));
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->dashboard_panel.y));
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->dashboard_panel.width));
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->dashboard_panel.height));
    hash = debug_hash_u64(hash, overlay->hovered_ui_region == DEBUG_UI_HOVER_DASHBOARD ? 1U : 0U);
    return hash;
}

static uint64_t debug_overlay_compute_inspector_signature(
    const DebugOverlay* overlay,
    const DebugEntityView* selected_entity,
    int viewport_width,
    int viewport_height
) {
    uint64_t hash = 1469598103934665603ULL;

    hash = debug_hash_i32(hash, viewport_width);
    hash = debug_hash_i32(hash, viewport_height);
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->inspector_panel.x));
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->inspector_panel.y));
    hash = debug_hash_u64(hash, overlay->inspector_collapsed ? 1U : 0U);
    hash = debug_hash_u64(hash, overlay->hovered_ui_region == DEBUG_UI_HOVER_INSPECTOR ? 1U : 0U);
    if (selected_entity != NULL) {
        hash = debug_hash_u64(hash, selected_entity->id);
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->position.x));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->position.y));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->velocity.x));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->velocity.y));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->angle_radians));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->extent_x));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->extent_y));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->radius));
        hash = debug_hash_u64(hash, selected_entity->is_circle ? 1U : 0U);
        hash = debug_hash_u64(hash, selected_entity->is_selected ? 1U : 0U);
        hash = debug_hash_u64(hash, selected_entity->is_sleeping ? 1U : 0U);
        hash = debug_hash_u64(hash, selected_entity->is_moving ? 1U : 0U);
    }

    return hash;
}

static uint64_t debug_overlay_compute_world_signature(
    const DebugOverlay* overlay,
    const Camera* camera,
    const DebugGridView* grid,
    const DebugEntityView* entities,
    size_t entity_count,
    const DebugCollisionView* collisions,
    size_t collision_count,
    uint64_t selected_entity_id,
    uint64_t hovered_entity_id
) {
    uint64_t hash = 1469598103934665603ULL;
    size_t index = 0U;

    if (overlay == NULL || camera == NULL) {
        return 0ULL;
    }

    hash = debug_hash_i32(hash, debug_round_tenths(camera->x));
    hash = debug_hash_i32(hash, debug_round_tenths(camera->y));
    hash = debug_hash_i32(hash, debug_round_tenths(camera->view_width));
    hash = debug_hash_i32(hash, debug_round_tenths(camera->view_height));
    hash = debug_hash_u64(hash, selected_entity_id);
    hash = debug_hash_u64(hash, hovered_entity_id);
    hash = debug_hash_u64(hash, entity_count);
    hash = debug_hash_u64(hash, collision_count);
    if (grid != NULL && grid->enabled) {
        hash = debug_hash_i32(hash, debug_round_tenths(grid->cell_size));
        hash = debug_hash_i32(hash, grid->span_min_x);
        hash = debug_hash_i32(hash, grid->span_max_x);
        hash = debug_hash_i32(hash, grid->span_min_y);
        hash = debug_hash_i32(hash, grid->span_max_y);
    }

    for (index = 0U; index < entity_count; ++index) {
        const DebugEntityView* entity = &entities[index];
        hash = debug_hash_u64(hash, entity->id);
        hash = debug_hash_i32(hash, debug_round_tenths(entity->position.x));
        hash = debug_hash_i32(hash, debug_round_tenths(entity->position.y));
        hash = debug_hash_i32(hash, debug_round_tenths(entity->angle_radians));
        hash = debug_hash_u64(hash, entity->is_sleeping ? 1U : 0U);
    }

    for (index = 0U; index < collision_count; ++index) {
        const DebugCollisionView* collision = &collisions[index];
        if (!collision->active) {
            continue;
        }
        hash = debug_hash_i32(hash, debug_round_tenths(collision->contact_point.x));
        hash = debug_hash_i32(hash, debug_round_tenths(collision->contact_point.y));
        hash = debug_hash_i32(hash, debug_round_tenths(collision->contact_normal.x));
        hash = debug_hash_i32(hash, debug_round_tenths(collision->contact_normal.y));
        hash = debug_hash_u64(hash, collision->sensor ? 1U : 0U);
        hash = debug_hash_u64(hash, collision->sleeping ? 1U : 0U);
    }

    return hash;
}

static void debug_overlay_draw_dashboard_contents(
    DebugOverlay *overlay,
    Target *target,
    const DebugOverlaySnapshot *snapshot,
    const EngineStatsSnapshot *stats,
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

    debug_draw_panel_frame(
        target,
        &overlay->dashboard_panel,
        "Performance",
        overlay->hovered_ui_region == DEBUG_UI_HOVER_DASHBOARD
    );
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

static void debug_overlay_draw_inspector_contents(
    DebugOverlay* overlay,
    Target* target,
    const DebugEntityView* selected_entity
) {
    char text[128];
    float left;
    float top;
    float right;

    if (overlay == NULL || target == NULL || selected_entity == NULL) {
        return;
    }

    if (overlay->inspector_collapsed) {
        debug_draw_inspector_rail(
            target,
            &overlay->inspector_panel,
            overlay->hovered_ui_region == DEBUG_UI_HOVER_INSPECTOR
        );
        return;
    }

    debug_draw_panel_frame(
        target,
        &overlay->inspector_panel,
        "Inspector",
        overlay->hovered_ui_region == DEBUG_UI_HOVER_INSPECTOR
    );
    left = overlay->inspector_panel.x + 12.0f;
    top = overlay->inspector_panel.y + 30.0f;
    right = overlay->inspector_panel.x + overlay->inspector_panel.width - 12.0f;

    snprintf(text, sizeof(text), "Entity %llu", (unsigned long long)selected_entity->id);
    target_text(target, left, top, text, (Color32){ 0xf2f8fcU });
    snprintf(text, sizeof(text), "%s", selected_entity->is_sleeping ? "Sleeping" : (selected_entity->is_moving ? "Moving" : "Idle"));
    target_text(target, right - 54.0f, top, text, selected_entity->is_sleeping ? (Color32){ 0x7f99adU } : (Color32){ 0x8fe8c4U });
    top += 18.0f;

    target_text(target, left, top, "Position", (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%.1f  %.1f", selected_entity->position.x, selected_entity->position.y);
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
    top += 16.0f;

    target_text(target, left, top, "Velocity", (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%.1f  %.1f", selected_entity->velocity.x, selected_entity->velocity.y);
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
    top += 16.0f;

    target_text(target, left, top, "Angle", (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%.2f rad", selected_entity->angle_radians);
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
    top += 16.0f;

    target_text(target, left, top, "Shape", (Color32){ 0x7f99adU });
    if (selected_entity->is_circle) {
        snprintf(text, sizeof(text), "Circle %.1f", selected_entity->radius);
    } else {
        snprintf(text, sizeof(text), "Box %.1f x %.1f", selected_entity->extent_x, selected_entity->extent_y);
    }
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
    top += 16.0f;

    target_text(target, left, top, "Flags", (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%s  %s",
        selected_entity->is_selected ? "Selected" : "Unselected",
        selected_entity->is_moving ? "Moving" : "Still");
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
}

void debug_overlay_draw_ui(
    DebugOverlay *overlay,
    RenderBackend *backend,
    const DebugOverlaySnapshot *snapshot,
    const EngineStatsSnapshot *stats,
    const DebugEntityView *selected_entity,
    int viewport_width,
    int viewport_height
) {
    Target target;
    bool has_selection;
    uint64_t dashboard_signature;
    uint64_t inspector_signature;
    uint64_t now_ms;
    double layout_started_ms = 0.0;
    double composite_started_ms = 0.0;

    if (overlay == NULL || !overlay->draw_ui || backend == NULL || snapshot == NULL || stats == NULL) {
        return;
    }

    has_selection = selected_entity != NULL;
    now_ms = (uint64_t)debug_overlay_now_ms();
    layout_started_ms = debug_overlay_now_ms();
    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    overlay->last_ui_layout_ms = debug_overlay_now_ms() - layout_started_ms;
    debug_overlay_tick_dashboard_state(overlay, snapshot, stats);
    dashboard_signature = debug_overlay_compute_dashboard_signature(overlay, viewport_width, viewport_height);
    inspector_signature = debug_overlay_compute_inspector_signature(overlay, selected_entity, viewport_width, viewport_height);
    if (backend->create_surface != NULL &&
        backend->destroy_surface != NULL &&
        backend->set_target != NULL &&
        backend->reset_target != NULL &&
        backend->clear != NULL) {
        if (overlay->dashboard_surface == NULL ||
            overlay->dashboard_surface_width != (int)overlay->dashboard_panel.width ||
            overlay->dashboard_surface_height != (int)overlay->dashboard_panel.height) {
            if (overlay->dashboard_surface != NULL) {
                overlay->dashboard_surface_backend->destroy_surface(overlay->dashboard_surface_backend->userdata, overlay->dashboard_surface);
            }
            overlay->dashboard_surface = backend->create_surface(
                backend->userdata,
                (int)overlay->dashboard_panel.width,
                (int)overlay->dashboard_panel.height
            );
            overlay->dashboard_surface_backend = backend;
            overlay->dashboard_surface_width = (int)overlay->dashboard_panel.width;
            overlay->dashboard_surface_height = (int)overlay->dashboard_panel.height;
            overlay->last_dashboard_signature = 0ULL;
        }

        if (overlay->inspector_surface == NULL ||
            overlay->inspector_surface_width != (int)(overlay->inspector_collapsed ? debug_inspector_collapsed_width() : overlay->inspector_panel.width) ||
            overlay->inspector_surface_height != (int)overlay->inspector_panel.height) {
            if (overlay->inspector_surface != NULL) {
                overlay->inspector_surface_backend->destroy_surface(overlay->inspector_surface_backend->userdata, overlay->inspector_surface);
            }
            overlay->inspector_surface = backend->create_surface(
                backend->userdata,
                (int)(overlay->inspector_collapsed ? debug_inspector_collapsed_width() : overlay->inspector_panel.width),
                (int)overlay->inspector_panel.height
            );
            overlay->inspector_surface_backend = backend;
            overlay->inspector_surface_width = (int)(overlay->inspector_collapsed ? debug_inspector_collapsed_width() : overlay->inspector_panel.width);
            overlay->inspector_surface_height = (int)overlay->inspector_panel.height;
            overlay->last_inspector_signature = 0ULL;
        }

        overlay->last_ui_draw_ms = 0.0;
        if (overlay->dashboard_surface != NULL && debug_overlay_should_redraw_panel(
                overlay->last_dashboard_signature,
                dashboard_signature,
                overlay->last_dashboard_redraw_at_ms,
                now_ms,
                debug_dashboard_redraw_interval_ms(),
                overlay->dashboard_panel.dragging)) {
            double redraw_started_ms = debug_overlay_now_ms();
            backend->set_target(backend->userdata, overlay->dashboard_surface);
            backend->clear(backend->userdata, (Color32){ 0x00000000U });
            target_init(&target, backend, -overlay->dashboard_panel.x, -overlay->dashboard_panel.y);
            debug_overlay_draw_dashboard_contents(overlay, &target, snapshot, stats, viewport_width, viewport_height);
            backend->reset_target(backend->userdata);
            overlay->last_dashboard_signature = dashboard_signature;
            overlay->last_dashboard_redraw_at_ms = now_ms;
            overlay->dashboard_redraw_count += 1U;
            overlay->last_dashboard_redraw_ms = debug_overlay_now_ms() - redraw_started_ms;
            overlay->last_ui_draw_ms += overlay->last_dashboard_redraw_ms;
        } else {
            overlay->dashboard_redraw_skip_count += 1U;
        }

        if (overlay->inspector_surface != NULL && debug_overlay_should_redraw_panel(
                overlay->last_inspector_signature,
                inspector_signature,
                overlay->last_inspector_redraw_at_ms,
                now_ms,
                debug_inspector_redraw_interval_ms(),
                overlay->inspector_panel.dragging)) {
            double redraw_started_ms = debug_overlay_now_ms();
            backend->set_target(backend->userdata, overlay->inspector_surface);
            backend->clear(backend->userdata, (Color32){ 0x00000000U });
            target_init(&target, backend, -overlay->inspector_panel.x, -overlay->inspector_panel.y);
            if (selected_entity != NULL) {
                debug_overlay_draw_inspector_contents(overlay, &target, selected_entity);
            }
            backend->reset_target(backend->userdata);
            overlay->last_inspector_signature = inspector_signature;
            overlay->last_inspector_redraw_at_ms = now_ms;
            overlay->inspector_redraw_count += 1U;
            overlay->last_inspector_redraw_ms = debug_overlay_now_ms() - redraw_started_ms;
            overlay->last_ui_draw_ms += overlay->last_inspector_redraw_ms;
        } else {
            overlay->inspector_redraw_skip_count += 1U;
        }

        if (overlay->dashboard_surface != NULL) {
            composite_started_ms = debug_overlay_now_ms();
            target_init(&target, backend, 0.0f, 0.0f);
            target_surface(&target, overlay->dashboard_surface, overlay->dashboard_panel.x, overlay->dashboard_panel.y);
            if (selected_entity != NULL && overlay->inspector_surface != NULL) {
                target_surface(&target, overlay->inspector_surface, overlay->inspector_panel.x, overlay->inspector_panel.y);
            }
            overlay->last_ui_composite_ms = debug_overlay_now_ms() - composite_started_ms;
            return;
        }
    }

    target_init(&target, backend, 0.0f, 0.0f);
    debug_overlay_draw_dashboard_contents(overlay, &target, snapshot, stats, viewport_width, viewport_height);
    if (selected_entity != NULL) {
        debug_overlay_draw_inspector_contents(overlay, &target, selected_entity);
    }
}
