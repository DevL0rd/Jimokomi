#include "SpatialGridInternal.h"

#include "EntityInternal.h"
#include "Components/TransformComponent.h"
#include "Components/ColliderComponent.h"

#include "../Core/Memory.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool spatial_grid_reserve_entries(SpatialGrid* grid, size_t required_capacity)
{
    SpatialGridEntry* entries = NULL;
    size_t new_capacity = 0;

    if (grid == NULL)
    {
        return false;
    }

    if (grid->entry_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = grid->entry_capacity == 0U ? 64U : grid->entry_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2U;
    }

    entries = (SpatialGridEntry*)realloc(grid->entries, sizeof(SpatialGridEntry) * new_capacity);
    if (entries == NULL)
    {
        return false;
    }

    grid->entries = entries;
    grid->entry_capacity = new_capacity;
    return true;
}

static bool spatial_grid_reserve_nodes(SpatialGrid* grid, size_t required_capacity)
{
    SpatialGridNode* nodes = NULL;
    size_t new_capacity = 0;

    if (grid == NULL)
    {
        return false;
    }

    if (grid->node_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = grid->node_capacity == 0U ? 128U : grid->node_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2U;
    }

    nodes = (SpatialGridNode*)realloc(grid->nodes, sizeof(SpatialGridNode) * new_capacity);
    if (nodes == NULL)
    {
        return false;
    }

    grid->nodes = nodes;
    grid->node_capacity = new_capacity;
    return true;
}

static bool spatial_grid_reserve_cells(SpatialGrid* grid, size_t required_cell_count)
{
    int* cell_heads = NULL;
    uint32_t* cell_entry_counts = NULL;
    uint8_t* dirty_cell_flags = NULL;

    if (grid == NULL)
    {
        return false;
    }

    cell_heads = (int*)realloc(grid->cell_heads, sizeof(int) * required_cell_count);
    if (cell_heads == NULL)
    {
        return false;
    }

    cell_entry_counts = (uint32_t*)realloc(grid->cell_entry_counts, sizeof(uint32_t) * required_cell_count);
    if (cell_entry_counts == NULL)
    {
        return false;
    }

    dirty_cell_flags = (uint8_t*)realloc(grid->dirty_cell_flags, sizeof(uint8_t) * required_cell_count);
    if (dirty_cell_flags == NULL)
    {
        return false;
    }

    grid->cell_heads = cell_heads;
    grid->cell_entry_counts = cell_entry_counts;
    grid->dirty_cell_flags = dirty_cell_flags;
    return true;
}

static bool spatial_grid_reserve_dirty_cells(SpatialGrid* grid, size_t required_capacity)
{
    uint32_t* dirty_cells = NULL;
    size_t new_capacity = 0U;

    if (grid == NULL)
    {
        return false;
    }

    if (grid->dirty_cell_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = grid->dirty_cell_capacity == 0U ? 64U : grid->dirty_cell_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2U;
    }

    dirty_cells = (uint32_t*)realloc(grid->dirty_cell_indices, sizeof(uint32_t) * new_capacity);
    if (dirty_cells == NULL)
    {
        return false;
    }

    grid->dirty_cell_indices = dirty_cells;
    grid->dirty_cell_capacity = new_capacity;
    return true;
}

static bool spatial_grid_get_entity_bounds(const Entity* entity, Aabb* out_bounds)
{
    const TransformComponent* transform = NULL;
    const ColliderComponent* collider = NULL;
    float half_width = 0.0f;
    float half_height = 0.0f;

    if (out_bounds == NULL || !Entity_IsActive(entity))
    {
        return false;
    }

    transform = (const TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
    if (transform == NULL)
    {
        return false;
    }

    collider = (const ColliderComponent*)Entity_GetFixedComponent(entity, COMPONENT_COLLIDER);
    if (collider != NULL)
    {
        if (collider->shape == COLLIDER_SHAPE_CIRCLE)
        {
            half_width = collider->radius;
            half_height = collider->radius;
        }
        else
        {
            half_width = collider->width * 0.5f;
            half_height = collider->height * 0.5f;
        }
    }

    out_bounds->min_x = transform->x - half_width;
    out_bounds->min_y = transform->y - half_height;
    out_bounds->max_x = transform->x + half_width;
    out_bounds->max_y = transform->y + half_height;
    return true;
}

bool spatial_grid_bounds_overlap(Aabb a, Aabb b)
{
    return a.min_x <= b.max_x && a.max_x >= b.min_x &&
           a.min_y <= b.max_y && a.max_y >= b.min_y;
}

static bool spatial_grid_spans_equal(SpatialGridCellSpan a, SpatialGridCellSpan b)
{
    return a.valid == b.valid &&
           a.min_cell_x == b.min_cell_x &&
           a.max_cell_x == b.max_cell_x &&
           a.min_cell_y == b.min_cell_y &&
           a.max_cell_y == b.max_cell_y;
}

int spatial_grid_cell_x(const SpatialGrid* grid, float x)
{
    float local_x = 0.0f;

    if (grid == NULL || grid->cell_size <= 0.0f || grid->columns == 0U)
    {
        return 0;
    }

    local_x = (x - grid->bounds.min_x) / grid->cell_size;
    return clamp_i((int)floorf(local_x), 0, (int)grid->columns - 1);
}

int spatial_grid_cell_y(const SpatialGrid* grid, float y)
{
    float local_y = 0.0f;

    if (grid == NULL || grid->cell_size <= 0.0f || grid->rows == 0U)
    {
        return 0;
    }

    local_y = (y - grid->bounds.min_y) / grid->cell_size;
    return clamp_i((int)floorf(local_y), 0, (int)grid->rows - 1);
}

static void spatial_grid_mark_dirty_cell(SpatialGrid* grid, uint32_t cell_index)
{
    if (grid == NULL || grid->dirty_cell_flags == NULL)
    {
        return;
    }

    if (grid->dirty_cell_flags[cell_index] != 0U)
    {
        return;
    }

    if (!spatial_grid_reserve_dirty_cells(grid, grid->dirty_cell_count + 1U))
    {
        return;
    }

    grid->dirty_cell_flags[cell_index] = 1U;
    grid->dirty_cell_indices[grid->dirty_cell_count++] = cell_index;
}

static void spatial_grid_mark_dirty_span(SpatialGrid* grid, SpatialGridCellSpan span)
{
    int cell_y = 0;

    if (grid == NULL || !span.valid)
    {
        return;
    }

    for (cell_y = span.min_cell_y; cell_y <= span.max_cell_y; ++cell_y)
    {
        int cell_x = 0;
        for (cell_x = span.min_cell_x; cell_x <= span.max_cell_x; ++cell_x)
        {
            uint32_t cell_index = (uint32_t)((size_t)cell_y * grid->columns + (size_t)cell_x);
            spatial_grid_mark_dirty_cell(grid, cell_index);
        }
    }
}

static void spatial_grid_unlink_entry_nodes(SpatialGrid* grid, SpatialGridEntry* entry)
{
    size_t node_offset = 0U;

    if (grid == NULL || entry == NULL || !entry->active)
    {
        return;
    }

    for (node_offset = 0U; node_offset < entry->node_count; ++node_offset)
    {
        SpatialGridNode* node = &grid->nodes[entry->node_start_index + node_offset];
        int cell_index = node->cell_index;

        if (cell_index < 0)
        {
            continue;
        }

        if (node->previous_index >= 0)
        {
            grid->nodes[node->previous_index].next_index = node->next_index;
        }
        else
        {
            grid->cell_heads[cell_index] = node->next_index;
        }

        if (node->next_index >= 0)
        {
            grid->nodes[node->next_index].previous_index = node->previous_index;
        }

        if (grid->cell_entry_counts[cell_index] > 0U)
        {
            grid->cell_entry_counts[cell_index] -= 1U;
        }

        node->cell_index = -1;
        node->previous_index = -1;
        node->next_index = -1;
    }

    spatial_grid_mark_dirty_span(grid, entry->span);
    entry->node_count = 0U;
}

static bool spatial_grid_link_entry_nodes(
    SpatialGrid* grid,
    SpatialGridEntry* entry,
    SpatialGridCellSpan span
)
{
    size_t required_nodes = 0U;
    size_t node_offset = 0U;
    int cell_y = 0;

    if (grid == NULL || entry == NULL || !span.valid)
    {
        return false;
    }

    required_nodes = (size_t)span.cell_count;
    if (!spatial_grid_reserve_nodes(grid, grid->node_count + required_nodes))
    {
        return false;
    }

    entry->span = span;
    entry->node_start_index = grid->node_count;
    entry->node_count = required_nodes;
    entry->active = true;

    for (cell_y = span.min_cell_y; cell_y <= span.max_cell_y; ++cell_y)
    {
        int cell_x = 0;
        for (cell_x = span.min_cell_x; cell_x <= span.max_cell_x; ++cell_x)
        {
            size_t cell_index = (size_t)cell_y * grid->columns + (size_t)cell_x;
            SpatialGridNode* node = &grid->nodes[entry->node_start_index + node_offset];
            int old_head = grid->cell_heads[cell_index];

            node->entry_index = (size_t)(entry - grid->entries);
            node->cell_index = (int)cell_index;
            node->previous_index = -1;
            node->next_index = old_head;
            if (old_head >= 0)
            {
                grid->nodes[old_head].previous_index = (int)(entry->node_start_index + node_offset);
            }
            grid->cell_heads[cell_index] = (int)(entry->node_start_index + node_offset);
            grid->cell_entry_counts[cell_index] += 1U;
            spatial_grid_mark_dirty_cell(grid, (uint32_t)cell_index);
            node_offset += 1U;
        }
    }

    grid->node_count += required_nodes;
    return true;
}

static void spatial_grid_release_entry(SpatialGrid* grid, SpatialGridEntry* entry)
{
    if (grid == NULL || entry == NULL)
    {
        return;
    }

    if (entry->active)
    {
        spatial_grid_unlink_entry_nodes(grid, entry);
    }

    entry->entity = NULL;
    entry->active = false;
    entry->bounds = (Aabb){ 0 };
    memset(&entry->span, 0, sizeof(entry->span));
    entry->node_start_index = 0U;
    entry->node_count = 0U;
    entry->last_query_stamp = 0U;
}

static SpatialGridEntry* spatial_grid_get_entry_for_entity(SpatialGrid* grid, Entity* entity)
{
    if (grid == NULL || entity == NULL || entity->spatial_grid_entry_index < 0)
    {
        return NULL;
    }

    if ((size_t)entity->spatial_grid_entry_index >= grid->entry_count)
    {
        entity->spatial_grid_entry_index = -1;
        return NULL;
    }

    if (grid->entries[entity->spatial_grid_entry_index].entity != entity)
    {
        entity->spatial_grid_entry_index = -1;
        return NULL;
    }

    return &grid->entries[entity->spatial_grid_entry_index];
}

static SpatialGridEntry* spatial_grid_acquire_entry(SpatialGrid* grid, Entity* entity)
{
    SpatialGridEntry* entry = NULL;

    if (grid == NULL || entity == NULL)
    {
        return NULL;
    }

    entry = spatial_grid_get_entry_for_entity(grid, entity);
    if (entry != NULL)
    {
        return entry;
    }

    if (!spatial_grid_reserve_entries(grid, grid->entry_count + 1U))
    {
        return NULL;
    }

    entry = &grid->entries[grid->entry_count];
    memset(entry, 0, sizeof(*entry));
    entry->entity = entity;
    entity->spatial_grid_entry_index = (int)grid->entry_count;
    grid->entry_count += 1U;
    return entry;
}

void SpatialGrid_Init(SpatialGrid* grid, float cell_size)
{
    if (grid == NULL)
    {
        return;
    }

    memset(grid, 0, sizeof(*grid));
    grid->cell_size = cell_size > 1.0f ? cell_size : 64.0f;
    grid->query_stamp = 1U;
}

void SpatialGrid_Dispose(SpatialGrid* grid)
{
    if (grid == NULL)
    {
        return;
    }

    free(grid->cell_heads);
    free(grid->cell_entry_counts);
    free(grid->dirty_cell_flags);
    free(grid->dirty_cell_indices);
    free(grid->entries);
    free(grid->nodes);
    memset(grid, 0, sizeof(*grid));
}

void SpatialGrid_SetBounds(SpatialGrid* grid, Aabb bounds)
{
    size_t required_cell_count = 0U;
    float width = 0.0f;
    float height = 0.0f;

    if (grid == NULL)
    {
        return;
    }

    width = bounds.max_x - bounds.min_x;
    height = bounds.max_y - bounds.min_y;
    if (width <= 0.0f || height <= 0.0f || grid->cell_size <= 0.0f)
    {
        SpatialGrid_ClearBounds(grid);
        return;
    }

    grid->bounds = bounds;
    grid->columns = (size_t)fmaxf(1.0f, ceilf(width / grid->cell_size));
    grid->rows = (size_t)fmaxf(1.0f, ceilf(height / grid->cell_size));
    required_cell_count = grid->columns * grid->rows;
    if (!spatial_grid_reserve_cells(grid, required_cell_count))
    {
        SpatialGrid_ClearBounds(grid);
        return;
    }

    memset(grid->cell_heads, 0xff, sizeof(int) * required_cell_count);
    memset(grid->cell_entry_counts, 0, sizeof(uint32_t) * required_cell_count);
    memset(grid->dirty_cell_flags, 0, sizeof(uint8_t) * required_cell_count);
    grid->enabled = true;
    grid->cell_count = required_cell_count;
    grid->entry_count = 0U;
    grid->node_count = 0U;
    grid->dirty_cell_count = 0U;
}

void SpatialGrid_ClearBounds(SpatialGrid* grid)
{
    if (grid == NULL)
    {
        return;
    }

    grid->enabled = false;
    grid->columns = 0U;
    grid->rows = 0U;
    grid->cell_count = 0U;
    grid->entry_count = 0U;
    grid->node_count = 0U;
    grid->dirty_cell_count = 0U;
}

bool SpatialGrid_GetCellSpanForAabb(const SpatialGrid* grid, Aabb bounds, SpatialGridCellSpan* out_span)
{
    SpatialGridCellSpan span = { 0 };
    int width = 0;
    int height = 0;

    if (grid == NULL || out_span == NULL || !grid->enabled || !spatial_grid_bounds_overlap(bounds, grid->bounds))
    {
        if (out_span != NULL)
        {
            *out_span = span;
        }
        return false;
    }

    span.min_cell_x = spatial_grid_cell_x(grid, bounds.min_x);
    span.max_cell_x = spatial_grid_cell_x(grid, bounds.max_x);
    span.min_cell_y = spatial_grid_cell_y(grid, bounds.min_y);
    span.max_cell_y = spatial_grid_cell_y(grid, bounds.max_y);
    width = span.max_cell_x - span.min_cell_x + 1;
    height = span.max_cell_y - span.min_cell_y + 1;
    span.cell_count = (uint32_t)(width * height);
    span.valid = true;
    *out_span = span;
    return true;
}

void SpatialGrid_Rebuild(SpatialGrid* grid, Entity* const* entities, size_t entity_count)
{
    size_t index = 0U;
    size_t cell_count = 0U;

    if (grid == NULL || !grid->enabled || entities == NULL)
    {
        return;
    }

    SpatialGrid_ClearDirtyCells(grid);
    cell_count = grid->columns * grid->rows;
    if (grid->cell_heads != NULL && cell_count > 0U)
    {
        memset(grid->cell_heads, 0xff, sizeof(int) * cell_count);
    }
    if (grid->cell_entry_counts != NULL && cell_count > 0U)
    {
        memset(grid->cell_entry_counts, 0, sizeof(uint32_t) * cell_count);
    }
    grid->entry_count = 0U;
    grid->node_count = 0U;

    for (index = 0U; index < entity_count; ++index)
    {
        Entity* entity = entities[index];
        Aabb bounds;
        SpatialGridEntry* entry = NULL;
        SpatialGridCellSpan span = { 0 };

        if (!spatial_grid_get_entity_bounds(entity, &bounds) || !spatial_grid_bounds_overlap(bounds, grid->bounds))
        {
            if (entity != NULL)
            {
                entity->spatial_grid_entry_index = -1;
            }
            continue;
        }

        if (!spatial_grid_reserve_entries(grid, grid->entry_count + 1U))
        {
            return;
        }

        entry = &grid->entries[grid->entry_count];
        memset(entry, 0, sizeof(*entry));
        entry->entity = entity;
        entry->bounds = bounds;
        entry->active = true;
        entry->last_query_stamp = 0U;
        entity->spatial_grid_entry_index = (int)grid->entry_count;
        SpatialGrid_GetCellSpanForAabb(grid, bounds, &span);
        if (!spatial_grid_link_entry_nodes(grid, entry, span))
        {
            entity->spatial_grid_entry_index = -1;
            return;
        }

        grid->entry_count += 1U;
    }
}

bool SpatialGrid_UpdateEntity(SpatialGrid* grid, Entity* entity)
{
    SpatialGridEntry* entry = NULL;
    SpatialGridCellSpan next_span = { 0 };
    Aabb next_bounds = { 0 };
    bool has_bounds = false;

    if (grid == NULL || !grid->enabled || entity == NULL)
    {
        return false;
    }

    has_bounds = spatial_grid_get_entity_bounds(entity, &next_bounds) &&
                 spatial_grid_bounds_overlap(next_bounds, grid->bounds) &&
                 SpatialGrid_GetCellSpanForAabb(grid, next_bounds, &next_span);

    entry = spatial_grid_get_entry_for_entity(grid, entity);
    if (!has_bounds)
    {
        if (entry != NULL)
        {
            spatial_grid_release_entry(grid, entry);
            entity->spatial_grid_entry_index = -1;
        }
        return true;
    }

    if (entry == NULL)
    {
        entry = spatial_grid_acquire_entry(grid, entity);
        if (entry == NULL)
        {
            return false;
        }
        entry->bounds = next_bounds;
        if (!spatial_grid_link_entry_nodes(grid, entry, next_span))
        {
            spatial_grid_release_entry(grid, entry);
            entity->spatial_grid_entry_index = -1;
            return false;
        }
    }
    else if (!entry->active || !spatial_grid_spans_equal(entry->span, next_span))
    {
        spatial_grid_unlink_entry_nodes(grid, entry);
        entry->bounds = next_bounds;
        if (!spatial_grid_link_entry_nodes(grid, entry, next_span))
        {
            spatial_grid_release_entry(grid, entry);
            entity->spatial_grid_entry_index = -1;
            return false;
        }
    }
    else
    {
        entry->bounds = next_bounds;
    }

    return true;
}

void SpatialGrid_RemoveEntity(SpatialGrid* grid, Entity* entity)
{
    SpatialGridEntry* entry = NULL;

    if (grid == NULL || entity == NULL)
    {
        return;
    }

    entry = spatial_grid_get_entry_for_entity(grid, entity);
    if (entry != NULL)
    {
        spatial_grid_release_entry(grid, entry);
    }
    entity->spatial_grid_entry_index = -1;
}

size_t SpatialGrid_GetDirtyCellSpans(const SpatialGrid* grid, SpatialGridCellSpan* results, size_t capacity)
{
    size_t result_count = 0U;
    size_t index = 0U;

    if (grid == NULL || results == NULL || capacity == 0U || !grid->enabled)
    {
        return 0U;
    }

    for (index = 0U; index < grid->dirty_cell_count && result_count < capacity; ++index)
    {
        uint32_t cell_index = grid->dirty_cell_indices[index];
        int cell_x = (int)(cell_index % grid->columns);
        int cell_y = (int)(cell_index / grid->columns);
        results[result_count++] = (SpatialGridCellSpan){
            .min_cell_x = cell_x,
            .max_cell_x = cell_x,
            .min_cell_y = cell_y,
            .max_cell_y = cell_y,
            .cell_count = 1U,
            .valid = true
        };
    }

    return result_count;
}

void SpatialGrid_ClearDirtyCells(SpatialGrid* grid)
{
    size_t index = 0U;

    if (grid == NULL)
    {
        return;
    }

    for (index = 0U; index < grid->dirty_cell_count; ++index)
    {
        if (grid->dirty_cell_indices[index] < grid->cell_count)
        {
            grid->dirty_cell_flags[grid->dirty_cell_indices[index]] = 0U;
        }
    }
    grid->dirty_cell_count = 0U;
}
