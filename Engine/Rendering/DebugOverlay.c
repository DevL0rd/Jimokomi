#include "DebugOverlayInternal.h"

#include "../Settings.h"

#include <string.h>

static uint64_t debug_history_push_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_history_push_interval_ms;
}

static uint64_t debug_dashboard_redraw_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_dashboard_redraw_interval_ms;
}

static uint64_t debug_inspector_redraw_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_inspector_redraw_interval_ms;
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
        debug_overlay_history_push(overlay, snapshot, stats);
        overlay->last_history_push_ms = now_ms;
        pushed_history = true;
    }

    if (pushed_history || overlay->last_one_percent_low_fps <= 0.0f) {
        overlay->last_one_percent_low_fps = debug_overlay_compute_one_percent_low_fps(overlay);
    }

    debug_overlay_update_display_values(overlay, snapshot, overlay->last_one_percent_low_fps);
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
    hash = debug_hash_i32(hash, debug_round_whole(overlay->display_physics_hz));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_visible_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_awake_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_total_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_sleeping_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->display_moved_body_count / 16.0f));
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
