#include "RendererInternal.h"

#include "ResourceManagerInternal.h"

bool renderer_is_item_visible(const Renderer* renderer, const SpriteRenderable* item)
{
    const VisualSourceResource* source = NULL;
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

    source = resource_manager_get_visual_source(renderer->resource_manager, item->visual_source_handle);
    if (source == NULL)
    {
        return false;
    }

    width = (float)source->width;
    height = (float)source->height;
    left = item->x - width * item->anchor_x;
    top = item->y - height * item->anchor_y;
    right = left + width;
    bottom = top + height;

    return right >= renderer->camera.x &&
           bottom >= renderer->camera.y &&
           left <= renderer->camera.x + renderer->camera.view_width &&
           top <= renderer->camera.y + renderer->camera.view_height;
}
