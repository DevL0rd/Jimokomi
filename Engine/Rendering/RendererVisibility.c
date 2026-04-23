#include "RendererInternal.h"

#include "RendererLifecycleInternal.h"
#include "ResourceManagerRegistry.h"

bool renderer_is_material_renderable_visible(const Renderer* renderer, const MaterialRenderable* item)
{
    const ProceduralTextureResource* source = NULL;
    const TextureResource* texture = NULL;
    const MaterialResource* material = NULL;
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    if (renderer == NULL || item == NULL || !item->visible)
    {
        return false;
    }

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
    }
    else
    {
        source = resource_manager_get_procedural_texture(renderer->lifecycle->resource_manager, material->value.procedural_texture_handle);
        if (source == NULL)
        {
            return false;
        }

        width = (float)source->width;
        height = (float)source->height;
    }
    left = item->x - width * item->anchor_x;
    top = item->y - height * item->anchor_y;
    right = left + width;
    bottom = top + height;

    return right >= renderer->lifecycle->camera.x &&
           bottom >= renderer->lifecycle->camera.y &&
           left <= renderer->lifecycle->camera.x + renderer->lifecycle->camera.view_width &&
           top <= renderer->lifecycle->camera.y + renderer->lifecycle->camera.view_height;
}
