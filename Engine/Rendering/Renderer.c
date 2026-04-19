#include "Renderer.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

static double renderer_now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static bool renderer_is_item_visible(const Renderer* renderer, const SpriteRenderable* item) {
    const VisualSourceResource* source;
    float left;
    float top;
    float right;
    float bottom;
    float width;
    float height;

    if (renderer == NULL || item == NULL || !item->visible) {
        return false;
    }

    source = resource_manager_get_visual_source(&renderer->resource_manager, item->visual_source_handle);
    if (source == NULL) {
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

static bool renderer_reserve_scratch(Renderer* renderer, size_t required_capacity) {
    SpriteRenderable* items;
    size_t next_capacity;

    if (renderer == NULL) {
        return false;
    }

    if (renderer->scratch_capacity >= required_capacity) {
        return true;
    }

    next_capacity = renderer->scratch_capacity > 0U ? renderer->scratch_capacity : 16U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    items = (SpriteRenderable*)realloc(renderer->scratch_items, next_capacity * sizeof(*items));
    if (items == NULL) {
        return false;
    }

    renderer->scratch_items = items;
    renderer->scratch_capacity = next_capacity;
    return true;
}

static void renderer_draw_sprite_body(Renderer *renderer, RenderBackend *backend, const SpriteRenderable *item, uint64_t now_ms) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    const ShaderResource* shader;
    const Surface* baked_body;
    const Surface* baked_overlay;
    Target target;
    ProceduralTextureContext context;
    Vec2 screen;
    Rect dest;
    Vec2 origin;
    float width;
    float height;
    double started_ms;

    if (renderer == NULL || backend == NULL || item == NULL || !item->visible) {
        return;
    }

    source = resource_manager_get_visual_source(&renderer->resource_manager, item->visual_source_handle);
    material = resource_manager_get_material(&renderer->resource_manager, item->material_handle);
    shader = resource_manager_get_shader(&renderer->resource_manager, item->shader_handle);
    if (source == NULL || material == NULL || source->draw_body == NULL) {
        return;
    }

    width = (float)source->width;
    height = (float)source->height;
    screen = camera_world_to_screen(&renderer->camera, (Vec2){ item->x, item->y });
    dest.x = screen.x - width * item->anchor_x;
    dest.y = screen.y - height * item->anchor_y;
    dest.w = width;
    dest.h = height;
    origin.x = width * item->anchor_x;
    origin.y = height * item->anchor_y;
    memset(&context, 0, sizeof(context));
    context.now_ms = now_ms;
    context.time_seconds = (float)now_ms / 1000.0f;
    context.animation_fps = source->animation_fps;
    context.frame_index = source->animation_fps > 0.0f
        ? (uint32_t)floorf((context.time_seconds * source->animation_fps))
        : 0U;
    context.angle_radians = item->angle_radians;
    context.material = &material->value;
    context.shader_style = shader != NULL ? shader->style : material->value.shader_style;
    if (source->bake_policy != BAKE_POLICY_DISABLED) {
        baked_body = resource_manager_get_baked_surface(
            &renderer->resource_manager,
            item->visual_source_handle,
            item->material_handle,
            item->shader_handle,
            context.frame_index,
            BAKED_SURFACE_PASS_BODY,
            item->user_data
        );
        if (baked_body != NULL) {
            started_ms = renderer_now_ms();
            target_init(&target, backend, 0.0f, 0.0f);
            target_surface_ex(&target, baked_body, dest, origin, item->angle_radians * (180.0f / 3.14159265359f));
            renderer->last_body_draw_ms += renderer_now_ms() - started_ms;
        } else {
            resource_manager_request_baked_surface(
                &renderer->resource_manager,
                item->visual_source_handle,
                item->material_handle,
                item->shader_handle,
                context.frame_index,
                BAKED_SURFACE_PASS_BODY
            );
            target_init(&target, backend, dest.x, dest.y);
            started_ms = renderer_now_ms();
            source->draw_body(&target, &context, item->user_data);
            renderer->last_body_draw_ms += renderer_now_ms() - started_ms;
        }
        if (source->draw_overlay != NULL) {
            baked_overlay = resource_manager_get_baked_surface(
                &renderer->resource_manager,
                item->visual_source_handle,
                item->material_handle,
                item->shader_handle,
                context.frame_index,
                BAKED_SURFACE_PASS_OVERLAY,
                item->user_data
            );
            if (baked_overlay != NULL) {
                started_ms = renderer_now_ms();
                target_init(&target, backend, 0.0f, 0.0f);
                target_surface(&target, baked_overlay, dest.x, dest.y);
                renderer->last_overlay_draw_ms += renderer_now_ms() - started_ms;
            } else {
                resource_manager_request_baked_surface(
                    &renderer->resource_manager,
                    item->visual_source_handle,
                    item->material_handle,
                    item->shader_handle,
                    context.frame_index,
                    BAKED_SURFACE_PASS_OVERLAY
                );
                target_init(&target, backend, dest.x, dest.y);
                started_ms = renderer_now_ms();
                source->draw_overlay(&target, &context, item->user_data);
                renderer->last_overlay_draw_ms += renderer_now_ms() - started_ms;
            }
            renderer->last_overlay_draw_count += 1U;
        }
    } else {
        target_init(&target, backend, dest.x, dest.y);
        started_ms = renderer_now_ms();
        source->draw_body(&target, &context, item->user_data);
        renderer->last_body_draw_ms += renderer_now_ms() - started_ms;
        if (source->draw_overlay != NULL) {
            target_init(&target, backend, dest.x, dest.y);
            started_ms = renderer_now_ms();
            source->draw_overlay(&target, &context, item->user_data);
            renderer->last_overlay_draw_ms += renderer_now_ms() - started_ms;
            renderer->last_overlay_draw_count += 1U;
        }
    }
    if (source->kind == VISUAL_KIND_PROCEDURAL_TEXTURE) {
        renderer->last_procedural_item_count += 1U;
    }
}

static int renderer_compare_item_layer(const void *left, const void *right) {
    const SpriteRenderable *a = (const SpriteRenderable *)left;
    const SpriteRenderable *b = (const SpriteRenderable *)right;
    return (a->layer > b->layer) - (a->layer < b->layer);
}

void renderer_init(Renderer *renderer, RenderBackend *backend, const RendererConfig *config) {
    (void)backend;
    if (renderer == NULL) {
        return;
    }
    memset(renderer, 0, sizeof(*renderer));
    renderer->view_width = config != NULL && config->view_width > 0 ? config->view_width : 480;
    renderer->view_height = config != NULL && config->view_height > 0 ? config->view_height : 270;
    resource_manager_init(&renderer->resource_manager, backend);
    if (config != NULL && config->prebake_budget_per_frame > 0U) {
        renderer->resource_manager.bake_budget_per_frame = config->prebake_budget_per_frame;
    }
    debug_overlay_init(&renderer->debug_overlay);
    camera_init(&renderer->camera, 0.0f, 0.0f, (float)renderer->view_width, (float)renderer->view_height, 0.12f);
}

void renderer_dispose(Renderer *renderer) {
    if (renderer == NULL) {
        return;
    }
    resource_manager_dispose(&renderer->resource_manager);
    debug_overlay_dispose(&renderer->debug_overlay);
    free(renderer->scratch_items);
    memset(renderer, 0, sizeof(*renderer));
}

void renderer_draw(Renderer *renderer, RenderBackend *backend, const RendererFrame *frame) {
    size_t index;
    bool needs_sort = false;
    const SpriteRenderable* draw_items = NULL;
    if (renderer == NULL || backend == NULL || frame == NULL) {
        return;
    }

    renderer->last_render_item_count = frame->item_count;
    renderer->last_sprite_draw_count = 0U;
    renderer->last_visible_item_count = 0U;
    renderer->last_overlay_draw_count = 0U;
    renderer->last_procedural_item_count = 0U;
    renderer->last_baked_surface_count = resource_manager_get_baked_surface_count(&renderer->resource_manager);
    renderer->last_sort_ms = 0.0;
    renderer->last_visibility_ms = 0.0;
    renderer->last_body_draw_ms = 0.0;
    renderer->last_overlay_draw_ms = 0.0;
    resource_manager_begin_frame(&renderer->resource_manager);

    if (frame->draw_sprites && frame->items != NULL && frame->item_count > 0U) {
        double started_ms;
        draw_items = frame->items;
        for (index = 1U; index < frame->item_count; ++index) {
            if (frame->items[index - 1U].layer != frame->items[index].layer) {
                needs_sort = true;
                break;
            }
        }
        if (needs_sort && renderer_reserve_scratch(renderer, frame->item_count)) {
            started_ms = renderer_now_ms();
            memcpy(renderer->scratch_items, frame->items, frame->item_count * sizeof(*renderer->scratch_items));
            qsort(renderer->scratch_items, frame->item_count, sizeof(*renderer->scratch_items), renderer_compare_item_layer);
            renderer->last_sort_ms = renderer_now_ms() - started_ms;
            draw_items = renderer->scratch_items;
        }
        for (index = 0U; index < frame->item_count; ++index) {
            started_ms = renderer_now_ms();
            if (!renderer_is_item_visible(renderer, &draw_items[index])) {
                renderer->last_visibility_ms += renderer_now_ms() - started_ms;
                continue;
            }
            renderer->last_visibility_ms += renderer_now_ms() - started_ms;
            renderer->last_visible_item_count += 1U;
            renderer_draw_sprite_body(renderer, backend, &draw_items[index], frame->now_ms);
            renderer->last_sprite_draw_count += 1U;
        }
        resource_manager_process_pending_bakes(&renderer->resource_manager);
    }

    if (frame->draw_debug) {
        debug_overlay_draw_world(
            &renderer->debug_overlay,
            backend,
            &renderer->camera,
            frame->debug_entities,
            frame->debug_entity_count,
            frame->debug_collisions,
            frame->debug_collision_count
        );
    }
}
