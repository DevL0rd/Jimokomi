#include "RendererInternal.h"

#include "ResourceManagerBake.h"
#include "ResourceManagerRegistry.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

bool renderer_reserve_instances(Renderer* renderer, size_t required_capacity)
{
    SurfaceDrawInstance* items = NULL;
    size_t next_capacity = 0U;

    if (renderer == NULL)
    {
        return false;
    }

    if (renderer->scratch_instance_capacity >= required_capacity)
    {
        return true;
    }

    next_capacity = renderer->scratch_instance_capacity > 0U ? renderer->scratch_instance_capacity : 32U;
    while (next_capacity < required_capacity)
    {
        next_capacity *= 2U;
    }

    items = (SurfaceDrawInstance*)realloc(renderer->scratch_instances, next_capacity * sizeof(*items));
    if (items == NULL)
    {
        return false;
    }

    renderer->scratch_instances = items;
    renderer->scratch_instance_capacity = next_capacity;
    return true;
}

bool renderer_prepare_batched_surface_draw(
    Renderer* renderer,
    const SpriteRenderable* item,
    uint64_t now_ms,
    RendererPreparedSurfaceDraw* prepared
)
{
    const VisualSourceResource* source = NULL;
    const MaterialResource* material = NULL;
    const Surface* baked_body = NULL;
    uint32_t frame_index = 0U;
    float width = 0.0f;
    float height = 0.0f;
    Vec2 screen_size;
    Vec2 screen;
    Rect dest;

    if (renderer == NULL || item == NULL || prepared == NULL)
    {
        return false;
    }

    memset(prepared, 0, sizeof(*prepared));

    source = resource_manager_get_visual_source(renderer->resource_manager, item->visual_source_handle);
    material = resource_manager_get_material(renderer->resource_manager, item->material_handle);
    if (source == NULL || material == NULL || source->draw_body == NULL)
    {
        return false;
    }
    if (source->draw_overlay != NULL || source->bake_policy == BAKE_POLICY_DISABLED)
    {
        return false;
    }
    if (fabsf(item->anchor_x - 0.5f) > 0.001f || fabsf(item->anchor_y - 0.5f) > 0.001f)
    {
        return false;
    }

    frame_index = source->animation_fps > 0.0f
        ? (uint32_t)floorf(((float)now_ms / 1000.0f) * source->animation_fps)
        : 0U;
    baked_body = resource_manager_get_baked_surface(
        renderer->resource_manager,
        item->visual_source_handle,
        item->material_handle,
        item->shader_handle,
        frame_index,
        BAKED_SURFACE_PASS_BODY,
        item->user_data
    );
    if (baked_body == NULL)
    {
        return false;
    }

    width = (float)source->width;
    height = (float)source->height;
    screen_size = camera_world_size_to_screen(&renderer->camera, (Vec2){ width, height });
    screen = camera_world_to_screen(&renderer->camera, (Vec2){ item->x, item->y });
    dest.x = screen.x - screen_size.x * item->anchor_x;
    dest.y = screen.y - screen_size.y * item->anchor_y;
    dest.w = screen_size.x;
    dest.h = screen_size.y;

    prepared->surface = baked_body;
    prepared->instance.dest = dest;
    prepared->instance.origin.x = screen_size.x * item->anchor_x;
    prepared->instance.origin.y = screen_size.y * item->anchor_y;
    prepared->instance.rotation_degrees = item->angle_radians * (180.0f / 3.14159265359f);
    prepared->instance.tint = source->bake_ignores_material
        ? (Color32){ material->value.base_color }
        : (Color32){ 0xffffffffU };
    prepared->valid = true;
    return true;
}

void renderer_flush_surface_batch(
    Renderer* renderer,
    RenderBackend* backend,
    RendererSurfaceBatch* batch
)
{
    double started_ms = 0.0;

    if (renderer == NULL || backend == NULL || batch == NULL)
    {
        return;
    }

    if (batch->surface == NULL || batch->count == 0U)
    {
        batch->surface = NULL;
        batch->tint.value = 0U;
        batch->count = 0U;
        return;
    }

    started_ms = renderer_now_ms();
    backend->draw_surface_batch(
        backend->userdata,
        batch->surface,
        renderer->scratch_instances,
        batch->count
    );
    renderer->last_instance_draw_ms += renderer_now_ms() - started_ms;
    renderer->last_instanced_batch_count += 1U;
    renderer->last_instanced_draw_count += batch->count;
    renderer->last_sprite_draw_count += batch->count;
    batch->surface = NULL;
    batch->tint.value = 0U;
    batch->count = 0U;
}
