#include "GridBackdrop.h"

#include <math.h>
#include <string.h>

static uint64_t grid_backdrop_hash_u64(uint64_t hash, uint64_t value)
{
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
    return hash;
}

void grid_backdrop_init(GridBackdropConfig* backdrop, float world_width, float world_height, float cell_size)
{
    if (backdrop == NULL)
    {
        return;
    }

    memset(backdrop, 0, sizeof(*backdrop));
    backdrop->world_width = world_width;
    backdrop->world_height = world_height;
    backdrop->cell_size = cell_size > 0.0f ? cell_size : 64.0f;
    backdrop->even_color = 0x111827U;
    backdrop->odd_color = 0x0f172aU;
    backdrop->border_color = 0xff4d5aU;
    grid_backdrop_mark_dirty(backdrop);
}

void grid_backdrop_mark_dirty(GridBackdropConfig* backdrop)
{
    if (backdrop == NULL)
    {
        return;
    }

    backdrop->dirty = true;
    backdrop->dirty_version += 1U;
}

void grid_backdrop_clear_dirty(GridBackdropConfig* backdrop)
{
    if (backdrop == NULL)
    {
        return;
    }

    backdrop->dirty = false;
}

uint64_t grid_backdrop_signature(const GridBackdropConfig* backdrop)
{
    uint64_t hash = 1469598103934665603ULL;

    if (backdrop == NULL)
    {
        return 0U;
    }

    hash = grid_backdrop_hash_u64(hash, backdrop->dirty_version);
    hash = grid_backdrop_hash_u64(hash, (uint64_t)(int32_t)(backdrop->world_width * 10.0f));
    hash = grid_backdrop_hash_u64(hash, (uint64_t)(int32_t)(backdrop->world_height * 10.0f));
    hash = grid_backdrop_hash_u64(hash, (uint64_t)(int32_t)(backdrop->cell_size * 10.0f));
    hash = grid_backdrop_hash_u64(hash, backdrop->even_color);
    hash = grid_backdrop_hash_u64(hash, backdrop->odd_color);
    hash = grid_backdrop_hash_u64(hash, backdrop->border_color);
    return hash;
}

uint64_t grid_backdrop_signature_from_user_data(void* user_data)
{
    return grid_backdrop_signature((const GridBackdropConfig*)user_data);
}

void grid_backdrop_draw(Target* target, const Camera* camera, void* user_data)
{
    const GridBackdropConfig* config = (const GridBackdropConfig*)user_data;
    const float tile_size = config != NULL && config->cell_size > 0.0f ? config->cell_size : 64.0f;
    const float world_width = config != NULL ? config->world_width : 1920.0f;
    const float world_height = config != NULL ? config->world_height : 1080.0f;
    uint32_t even_color = config != NULL ? config->even_color : 0x111827U;
    uint32_t odd_color = config != NULL ? config->odd_color : 0x0f172aU;
    uint32_t border_color = config != NULL ? config->border_color : 0xff4d5aU;
    int start_x;
    int end_x;
    int start_y;
    int end_y;
    int x;
    int y;

    if (target == NULL || camera == NULL)
    {
        return;
    }

    start_x = (int)floorf(camera->x / tile_size) - 1;
    end_x = (int)ceilf((camera->x + camera->view_width) / tile_size) + 1;
    start_y = (int)floorf(camera->y / tile_size) - 1;
    end_y = (int)ceilf((camera->y + camera->view_height) / tile_size) + 1;

    for (y = start_y; y <= end_y; ++y)
    {
        for (x = start_x; x <= end_x; ++x)
        {
            Rect cell = { (float)x * tile_size, (float)y * tile_size, tile_size, tile_size };
            Vec2 top_left = camera_world_to_screen(camera, (Vec2){ cell.x, cell.y });
            Vec2 screen_size = camera_world_size_to_screen(camera, (Vec2){ cell.w, cell.h });
            Rect screen_cell = { top_left.x, top_left.y, screen_size.x, screen_size.y };
            bool even = ((x + y) & 1) == 0;

            target_rect_filled(target, screen_cell, (Color32){ even ? even_color : odd_color });
        }
    }

    {
        Vec2 top_left = camera_world_to_screen(camera, (Vec2){ 0.0f, 0.0f });
        Vec2 screen_size = camera_world_size_to_screen(camera, (Vec2){ world_width, world_height });
        target_rect(target, (Rect){ top_left.x, top_left.y, screen_size.x, screen_size.y }, (Color32){ border_color });
    }
}
