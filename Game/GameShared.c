#include "Game/GameInternal.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>

float game_lerp(float a, float b, float alpha) {
    return a + ((b - a) * alpha);
}

bool game_camera_rect_changed(
    const GameState* game,
    const Camera* camera
) {
    if (game == NULL || camera == NULL) {
        return true;
    }

    return !game->hover_cache_valid ||
           camera->x != game->last_hover_camera_x ||
           camera->y != game->last_hover_camera_y ||
           camera->view_width != game->last_hover_camera_view_width ||
           camera->view_height != game->last_hover_camera_view_height;
}

float game_random_range(float min_value, float max_value) {
    float t = (float)rand() / (float)RAND_MAX;
    return min_value + ((max_value - min_value) * t);
}

uint32_t game_world_cell_x(float x) {
    return (uint32_t)clamp_i((int)floorf(x / GRID_CELL_SIZE), 0, (int)(WORLD_WIDTH / GRID_CELL_SIZE));
}

uint32_t game_world_cell_y(float y) {
    return (uint32_t)clamp_i((int)floorf(y / GRID_CELL_SIZE), 0, (int)(WORLD_HEIGHT / GRID_CELL_SIZE));
}

uint64_t game_hash_u64(uint64_t hash, uint64_t value) {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
    return hash;
}

uint64_t game_backdrop_signature(const WorldBackdropConfig* backdrop) {
    uint64_t hash = 1469598103934665603ULL;

    if (backdrop == NULL) {
        return 0U;
    }

    hash = game_hash_u64(hash, backdrop->dirty_version);
    hash = game_hash_u64(hash, (uint64_t)(int32_t)(backdrop->world_width * 10.0f));
    hash = game_hash_u64(hash, (uint64_t)(int32_t)(backdrop->world_height * 10.0f));
    hash = game_hash_u64(hash, (uint64_t)(int32_t)(backdrop->cell_size * 10.0f));
    return hash;
}

void game_mark_backdrop_dirty(WorldBackdropConfig* backdrop) {
    if (backdrop == NULL) {
        return;
    }

    backdrop->dirty = true;
    backdrop->dirty_version += 1U;
}

void game_clear_backdrop_dirty(WorldBackdropConfig* backdrop) {
    if (backdrop == NULL) {
        return;
    }

    backdrop->dirty = false;
}

void game_mark_ball_visual_state_dirty(Ball* ball) {
    if (ball == NULL || ball->entity == NULL) {
        return;
    }

    ball->visual_state_dirty = true;
    ball->visual_state_version += 1U;
    Entity_MarkDirty(ball->entity, ENTITY_DIRTY_VISUAL_STATE);
}

void game_clear_ball_visual_state_dirty(Ball* ball) {
    if (ball == NULL || ball->entity == NULL) {
        return;
    }

    ball->visual_state_dirty = false;
    Entity_ClearDirty(ball->entity, ENTITY_DIRTY_VISUAL_STATE);
}

void game_mark_ball_highlight_state(Ball* ball, bool selected, bool hovered) {
    if (ball == NULL || ball->entity == NULL) {
        return;
    }

    if (ball->is_selected == selected && ball->is_hovered == hovered) {
        return;
    }

    ball->is_selected = selected;
    ball->is_hovered = hovered;
    ball->highlight_state_dirty = true;
    ball->highlight_state_version += 1U;
    Entity_MarkDirty(ball->entity, ENTITY_DIRTY_SELECTION_HIGHLIGHT);
}

void game_clear_ball_highlight_dirty(Ball* ball) {
    if (ball == NULL || ball->entity == NULL) {
        return;
    }

    ball->highlight_state_dirty = false;
    Entity_ClearDirty(ball->entity, ENTITY_DIRTY_SELECTION_HIGHLIGHT);
}

void game_sync_ball_highlight_state(GameState* game) {
    size_t index;

    if (game == NULL) {
        return;
    }

    for (index = 0U; index < BALL_COUNT; ++index) {
        Ball* ball = &game->balls[index];
        if (ball->entity == NULL) {
            continue;
        }

        game_mark_ball_highlight_state(
            ball,
            index == game->selected_ball_index,
            index == game->hovered_ball_index
        );
    }
}

float game_compute_render_alpha(const GameState* game) {
    if (game == NULL || game->sim_step_dt_seconds <= 0.0) {
        return 1.0f;
    }

    return clamp_f(
        (float)(game->accumulator_seconds / game->sim_step_dt_seconds),
        0.0f,
        1.0f
    );
}

float game_lerp_angle(float a, float b, float alpha) {
    float delta = fmodf(b - a, 6.28318530718f);
    if (delta > 3.14159265359f) {
        delta -= 6.28318530718f;
    } else if (delta < -3.14159265359f) {
        delta += 6.28318530718f;
    }
    return a + delta * alpha;
}

double game_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

void game_sleep_ms(uint32_t milliseconds) {
    struct timespec request;

    request.tv_sec = (time_t)(milliseconds / 1000U);
    request.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    nanosleep(&request, NULL);
}
