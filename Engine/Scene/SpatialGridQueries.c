#include "SpatialGridInternal.h"

#include "EntityInternal.h"

#include <math.h>

static float spatial_grid_clamp_f(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

static bool spatial_grid_bounds_overlap_circle(Aabb bounds, Vec2 center, float radius)
{
    float nearest_x = spatial_grid_clamp_f(center.x, bounds.min_x, bounds.max_x);
    float nearest_y = spatial_grid_clamp_f(center.y, bounds.min_y, bounds.max_y);
    float delta_x = center.x - nearest_x;
    float delta_y = center.y - nearest_y;

    return (delta_x * delta_x) + (delta_y * delta_y) <= radius * radius;
}

static bool spatial_grid_bounds_overlap_segment(Aabb bounds, Vec2 start, Vec2 end)
{
    float t_min = 0.0f;
    float t_max = 1.0f;
    float delta_x = end.x - start.x;
    float delta_y = end.y - start.y;

    if (start.x >= bounds.min_x && start.x <= bounds.max_x && start.y >= bounds.min_y && start.y <= bounds.max_y)
    {
        return true;
    }

    if (fabsf(delta_x) <= 0.00001f)
    {
        if (start.x < bounds.min_x || start.x > bounds.max_x)
        {
            return false;
        }
    }
    else
    {
        float inv_dx = 1.0f / delta_x;
        float tx1 = (bounds.min_x - start.x) * inv_dx;
        float tx2 = (bounds.max_x - start.x) * inv_dx;
        float tx_min = fminf(tx1, tx2);
        float tx_max = fmaxf(tx1, tx2);
        t_min = fmaxf(t_min, tx_min);
        t_max = fminf(t_max, tx_max);
        if (t_min > t_max)
        {
            return false;
        }
    }

    if (fabsf(delta_y) <= 0.00001f)
    {
        if (start.y < bounds.min_y || start.y > bounds.max_y)
        {
            return false;
        }
    }
    else
    {
        float inv_dy = 1.0f / delta_y;
        float ty1 = (bounds.min_y - start.y) * inv_dy;
        float ty2 = (bounds.max_y - start.y) * inv_dy;
        float ty_min = fminf(ty1, ty2);
        float ty_max = fmaxf(ty1, ty2);
        t_min = fmaxf(t_min, ty_min);
        t_max = fminf(t_max, ty_max);
        if (t_min > t_max)
        {
            return false;
        }
    }

    return t_max >= 0.0f && t_min <= 1.0f;
}

static void spatial_grid_begin_query(SpatialGrid* grid)
{
    grid->query_stamp += 1U;
    if (grid->query_stamp == 0U)
    {
        size_t index = 0U;
        grid->query_stamp = 1U;
        for (index = 0U; index < grid->entry_count; ++index)
        {
            grid->entries[index].last_query_stamp = 0U;
        }
    }
}

size_t SpatialGrid_QueryAabb(SpatialGrid* grid, Aabb bounds, Entity** results, size_t capacity)
{
    size_t result_count = 0U;
    int min_cell_x = 0;
    int max_cell_x = 0;
    int min_cell_y = 0;
    int max_cell_y = 0;
    int cell_y = 0;

    if (grid == NULL || !grid->enabled || results == NULL || capacity == 0U || !spatial_grid_bounds_overlap(bounds, grid->bounds))
    {
        return 0U;
    }

    spatial_grid_begin_query(grid);

    min_cell_x = spatial_grid_cell_x(grid, bounds.min_x);
    max_cell_x = spatial_grid_cell_x(grid, bounds.max_x);
    min_cell_y = spatial_grid_cell_y(grid, bounds.min_y);
    max_cell_y = spatial_grid_cell_y(grid, bounds.max_y);

    for (cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y)
    {
        int cell_x = 0;
        for (cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x)
        {
            size_t cell_index = (size_t)cell_y * grid->columns + (size_t)cell_x;
            int node_index = grid->cell_heads[cell_index];
            while (node_index >= 0 && result_count < capacity)
            {
                SpatialGridNode* node = &grid->nodes[node_index];
                SpatialGridEntry* entry = &grid->entries[node->entry_index];

                if (entry->last_query_stamp == grid->query_stamp)
                {
                    node_index = node->next_index;
                    continue;
                }

                if (spatial_grid_bounds_overlap(entry->bounds, bounds))
                {
                    entry->last_query_stamp = grid->query_stamp;
                    results[result_count++] = entry->entity;
                }
                node_index = node->next_index;
            }
        }
    }

    return result_count;
}

size_t SpatialGrid_QueryCircle(SpatialGrid* grid, Vec2 center, float radius, Entity** results, size_t capacity)
{
    size_t result_count = 0U;
    int min_cell_x = 0;
    int max_cell_x = 0;
    int min_cell_y = 0;
    int max_cell_y = 0;
    int cell_y = 0;
    Aabb query_bounds;

    if (grid == NULL || !grid->enabled || results == NULL || capacity == 0U || radius < 0.0f)
    {
        return 0U;
    }

    query_bounds.min_x = center.x - radius;
    query_bounds.min_y = center.y - radius;
    query_bounds.max_x = center.x + radius;
    query_bounds.max_y = center.y + radius;
    if (!spatial_grid_bounds_overlap(query_bounds, grid->bounds))
    {
        return 0U;
    }

    spatial_grid_begin_query(grid);

    min_cell_x = spatial_grid_cell_x(grid, query_bounds.min_x);
    max_cell_x = spatial_grid_cell_x(grid, query_bounds.max_x);
    min_cell_y = spatial_grid_cell_y(grid, query_bounds.min_y);
    max_cell_y = spatial_grid_cell_y(grid, query_bounds.max_y);

    for (cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y)
    {
        int cell_x = 0;
        for (cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x)
        {
            size_t cell_index = (size_t)cell_y * grid->columns + (size_t)cell_x;
            int node_index = grid->cell_heads[cell_index];
            while (node_index >= 0 && result_count < capacity)
            {
                SpatialGridNode* node = &grid->nodes[node_index];
                SpatialGridEntry* entry = &grid->entries[node->entry_index];

                if (entry->last_query_stamp == grid->query_stamp)
                {
                    node_index = node->next_index;
                    continue;
                }

                if (spatial_grid_bounds_overlap_circle(entry->bounds, center, radius))
                {
                    entry->last_query_stamp = grid->query_stamp;
                    results[result_count++] = entry->entity;
                }
                node_index = node->next_index;
            }
        }
    }

    return result_count;
}

size_t SpatialGrid_QuerySegment(SpatialGrid* grid, Vec2 start, Vec2 end, Entity** results, size_t capacity)
{
    size_t result_count = 0U;
    int min_cell_x = 0;
    int max_cell_x = 0;
    int min_cell_y = 0;
    int max_cell_y = 0;
    int cell_y = 0;
    Aabb query_bounds;

    if (grid == NULL || !grid->enabled || results == NULL || capacity == 0U)
    {
        return 0U;
    }

    query_bounds.min_x = fminf(start.x, end.x);
    query_bounds.min_y = fminf(start.y, end.y);
    query_bounds.max_x = fmaxf(start.x, end.x);
    query_bounds.max_y = fmaxf(start.y, end.y);
    if (!spatial_grid_bounds_overlap(query_bounds, grid->bounds))
    {
        return 0U;
    }

    spatial_grid_begin_query(grid);

    min_cell_x = spatial_grid_cell_x(grid, query_bounds.min_x);
    max_cell_x = spatial_grid_cell_x(grid, query_bounds.max_x);
    min_cell_y = spatial_grid_cell_y(grid, query_bounds.min_y);
    max_cell_y = spatial_grid_cell_y(grid, query_bounds.max_y);

    for (cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y)
    {
        int cell_x = 0;
        for (cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x)
        {
            size_t cell_index = (size_t)cell_y * grid->columns + (size_t)cell_x;
            int node_index = grid->cell_heads[cell_index];
            while (node_index >= 0 && result_count < capacity)
            {
                SpatialGridNode* node = &grid->nodes[node_index];
                SpatialGridEntry* entry = &grid->entries[node->entry_index];

                if (entry->last_query_stamp == grid->query_stamp)
                {
                    node_index = node->next_index;
                    continue;
                }

                if (spatial_grid_bounds_overlap_segment(entry->bounds, start, end))
                {
                    entry->last_query_stamp = grid->query_stamp;
                    results[result_count++] = entry->entity;
                }
                node_index = node->next_index;
            }
        }
    }

    return result_count;
}
