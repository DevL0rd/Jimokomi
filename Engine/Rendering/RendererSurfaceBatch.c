#include "RendererInternal.h"

#include "GeneratedFrame.h"
#include "RendererLifecycleInternal.h"
#include "RendererScratchInternal.h"
#include "RendererStatsInternal.h"
#include "ResourceManagerBake.h"
#include "ResourceManagerRegistry.h"

#include "../Core/Memory.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

bool renderer_reserve_instances(Renderer* renderer, size_t required_capacity)
{
    TextureDrawInstance* material_renderables = NULL;
    size_t next_capacity = 0U;

    if (renderer == NULL)
    {
        return false;
    }

    if (renderer->scratch->scratch_instance_capacity >= required_capacity)
    {
        return true;
    }

    next_capacity = renderer->scratch->scratch_instance_capacity > 0U ? renderer->scratch->scratch_instance_capacity : 32U;
    while (next_capacity < required_capacity)
    {
        next_capacity *= 2U;
    }

    material_renderables = (TextureDrawInstance*)realloc(renderer->scratch->scratch_instances, next_capacity * sizeof(*material_renderables));
    if (material_renderables == NULL)
    {
        return false;
    }

    renderer->scratch->scratch_instances = material_renderables;
    renderer->scratch->scratch_instance_capacity = next_capacity;
    return true;
}

bool renderer_prepare_batched_material_draw(
    Renderer* renderer,
    const MaterialRenderable* item,
    uint64_t now_ms,
    RendererPreparedTextureDraw* prepared
)
{
    const ProceduralTextureResource* source = NULL;
    const MaterialResource* material = NULL;
    const TextureResource* texture = NULL;
    const Texture* baked_body = NULL;
    uint32_t frame_index = 0U;
    float width = 0.0f;
    float height = 0.0f;
    Vec2 screen_size;
    Vec2 screen;
    Rect dest;
    Color32 item_tint;

    if (renderer == NULL || item == NULL || prepared == NULL)
    {
        return false;
    }

    memset(prepared, 0, sizeof(*prepared));

    material = resource_manager_get_material(renderer->lifecycle->resource_manager, item->material_handle);
    if (material == NULL)
    {
        return false;
    }

    texture = resource_manager_get_texture(renderer->lifecycle->resource_manager, material->value.texture_handle);
    if (texture != NULL && texture->texture != NULL)
    {
        width = (float)texture->texture->width;
        height = (float)texture->texture->height;
        screen_size = camera_world_size_to_screen(&renderer->lifecycle->camera, (Vec2){ width, height });
        screen = camera_world_to_screen(&renderer->lifecycle->camera, (Vec2){ item->x, item->y });
        dest.x = screen.x - screen_size.x * item->anchor_x;
        dest.y = screen.y - screen_size.y * item->anchor_y;
        dest.w = screen_size.x;
        dest.h = screen_size.y;
        prepared->texture = texture->texture;
        prepared->instance.dest = dest;
        prepared->instance.origin.x = screen_size.x * item->anchor_x;
        prepared->instance.origin.y = screen_size.y * item->anchor_y;
        prepared->instance.rotation_degrees = item->angle_radians * (180.0f / 3.14159265359f);
        prepared->instance.tint = item->tint.value != 0U ? item->tint : (Color32){ 0xffffffffU };
        prepared->valid = true;
        return true;
    }

    source = resource_manager_get_procedural_texture(renderer->lifecycle->resource_manager, material->value.procedural_texture_handle);
    if (source == NULL || material == NULL || source->draw_body == NULL)
    {
        return false;
    }
    if (source->draw_overlay != NULL || source->frames.cache_policy == BAKE_POLICY_DISABLED)
    {
        return false;
    }
    if (fabsf(item->anchor_x - 0.5f) > 0.001f || fabsf(item->anchor_y - 0.5f) > 0.001f)
    {
        return false;
    }

    frame_index = generated_frame_animation_index(&source->frames, now_ms);
    baked_body = resource_manager_get_or_create_baked_texture(
        renderer->lifecycle->resource_manager,
        material->value.procedural_texture_handle,
        item->material_handle,
        material->value.shader_handle,
        frame_index,
        BAKED_TEXTURE_PASS_BODY,
        item->user_data
    );
    if (baked_body == NULL)
    {
        return false;
    }

    width = (float)source->width;
    height = (float)source->height;
    screen_size = camera_world_size_to_screen(&renderer->lifecycle->camera, (Vec2){ width, height });
    screen = camera_world_to_screen(&renderer->lifecycle->camera, (Vec2){ item->x, item->y });
    dest.x = screen.x - screen_size.x * item->anchor_x;
    dest.y = screen.y - screen_size.y * item->anchor_y;
    dest.w = screen_size.x;
    dest.h = screen_size.y;
    item_tint = item->tint.value != 0U ? item->tint : (Color32){ 0xffffffffU };
    if (source->bake_ignores_material && item_tint.value == 0xffffffffU)
    {
        item_tint = (Color32){ material->value.base_color != 0U ? material->value.base_color : 0xffffffffU };
    }

    prepared->texture = baked_body;
    prepared->instance.dest = dest;
    prepared->instance.origin.x = screen_size.x * item->anchor_x;
    prepared->instance.origin.y = screen_size.y * item->anchor_y;
    prepared->instance.rotation_degrees = item->angle_radians * (180.0f / 3.14159265359f);
    prepared->instance.tint = item_tint;
    prepared->valid = true;
    return true;
}

void renderer_flush_texture_batch(
    Renderer* renderer,
    RenderBackend* backend,
    RendererTextureBatch* batch
)
{
    double started_ms = 0.0;

    if (renderer == NULL || backend == NULL || batch == NULL)
    {
        return;
    }

    if (batch->texture == NULL || batch->count == 0U)
    {
        batch->texture = NULL;
        batch->tint.value = 0U;
        batch->count = 0U;
        return;
    }

    started_ms = renderer_now_ms();
    backend->draw_texture_batch(
        backend->userdata,
        batch->texture,
        renderer->scratch->scratch_instances,
        batch->count
    );
    renderer->stats->last_instance_draw_ms += renderer_now_ms() - started_ms;
    renderer->stats->last_instanced_batch_count += 1U;
    renderer->stats->last_instanced_draw_count += batch->count;
    renderer->stats->last_material_renderable_draw_count += batch->count;
    batch->texture = NULL;
    batch->tint.value = 0U;
    batch->count = 0U;
}
