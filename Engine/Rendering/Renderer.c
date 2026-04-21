#include "RendererInternal.h"

#include "DebugOverlayInternal.h"
#include "ResourceManagerInternal.h"
#include "ResourceCommandQueue.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

static double renderer_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static void renderer_draw_sprite_body(Renderer *renderer, RenderBackend *backend, const SpriteRenderable *item, uint64_t now_ms);

typedef struct RendererPreparedSurfaceDraw {
    const Surface* surface;
    SurfaceDrawInstance instance;
    bool valid;
} RendererPreparedSurfaceDraw;

typedef struct RendererSurfaceBatch {
    const Surface* surface;
    Color32 tint;
    size_t count;
} RendererSurfaceBatch;

static uint64_t renderer_hash_u64(uint64_t hash, uint64_t value) {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
    return hash;
}

static uint64_t renderer_compute_overlay_signature(const RendererFrame* frame) {
    uint64_t hash = 2166136261ULL;

    if (frame == NULL) {
        return 0U;
    }

    hash = renderer_hash_u64(hash, frame->debug_entity_count);
    hash = renderer_hash_u64(hash, frame->debug_collision_count);
    hash = renderer_hash_u64(hash, frame->selected_debug_entity_id);
    hash = renderer_hash_u64(hash, frame->hovered_debug_entity_id);
    hash = renderer_hash_u64(hash, frame->draw_debug ? 1U : 0U);
    return hash;
}

static void renderer_compute_frame_signatures(
    const RendererFrame* frame,
    uint64_t* out_frame_signature,
    uint64_t* out_sort_signature,
    uint64_t* out_instance_signature,
    bool* out_items_sorted_by_layer
) {
    uint64_t frame_hash = 1469598103934665603ULL;
    uint64_t sort_hash = 1099511628211ULL;
    uint64_t instance_hash = 88172645463325252ULL;
    size_t index;
    int previous_layer = 0;
    bool items_sorted_by_layer = true;

    if (frame == NULL) {
        if (out_frame_signature != NULL) {
            *out_frame_signature = 0U;
        }
        if (out_sort_signature != NULL) {
            *out_sort_signature = 0U;
        }
        if (out_instance_signature != NULL) {
            *out_instance_signature = 0U;
        }
        if (out_items_sorted_by_layer != NULL) {
            *out_items_sorted_by_layer = true;
        }
        return;
    }

    frame_hash = renderer_hash_u64(frame_hash, frame->item_count);
    frame_hash = renderer_hash_u64(frame_hash, frame->draw_sprites ? 1U : 0U);
    frame_hash = renderer_hash_u64(frame_hash, frame->draw_debug ? 1U : 0U);
    frame_hash = renderer_hash_u64(frame_hash, frame->metadata_signature);
    if (frame->item_signatures_valid) {
        frame_hash = renderer_hash_u64(frame_hash, frame->item_frame_signature);
        sort_hash = renderer_hash_u64(sort_hash, frame->item_sort_signature);
        instance_hash = renderer_hash_u64(instance_hash, frame->item_instance_signature);
        items_sorted_by_layer = frame->items_sorted_by_layer;
    } else {
        sort_hash = renderer_hash_u64(sort_hash, frame->item_count);
        instance_hash = renderer_hash_u64(instance_hash, frame->item_count);
        for (index = 0U; index < frame->item_count; ++index) {
            const SpriteRenderable* item = &frame->items[index];

            if (index > 0U && item->layer < previous_layer) {
                items_sorted_by_layer = false;
            }
            previous_layer = item->layer;
            frame_hash = renderer_hash_u64(frame_hash, (uint64_t)(int64_t)item->layer);
            frame_hash = renderer_hash_u64(frame_hash, item->visible ? 1U : 0U);
            frame_hash = renderer_hash_u64(frame_hash, item->visual_source_handle.id);
            frame_hash = renderer_hash_u64(frame_hash, item->material_handle.id);
            frame_hash = renderer_hash_u64(frame_hash, item->shader_handle.id);
            sort_hash = renderer_hash_u64(sort_hash, (uint64_t)(int64_t)item->layer);
            instance_hash = renderer_hash_u64(instance_hash, item->visual_source_handle.id);
            instance_hash = renderer_hash_u64(instance_hash, item->material_handle.id);
            instance_hash = renderer_hash_u64(instance_hash, item->shader_handle.id);
        }
    }

    if (out_frame_signature != NULL) {
        *out_frame_signature = frame_hash;
    }
    if (out_sort_signature != NULL) {
        *out_sort_signature = sort_hash;
    }
    if (out_instance_signature != NULL) {
        *out_instance_signature = instance_hash;
    }
    if (out_items_sorted_by_layer != NULL) {
        *out_items_sorted_by_layer = items_sorted_by_layer;
    }
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

    source = resource_manager_get_visual_source(renderer->resource_manager, item->visual_source_handle);
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

static bool renderer_reserve_instances(Renderer* renderer, size_t required_capacity) {
    SurfaceDrawInstance* items;
    size_t next_capacity;

    if (renderer == NULL) {
        return false;
    }

    if (renderer->scratch_instance_capacity >= required_capacity) {
        return true;
    }

    next_capacity = renderer->scratch_instance_capacity > 0U ? renderer->scratch_instance_capacity : 32U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    items = (SurfaceDrawInstance*)realloc(renderer->scratch_instances, next_capacity * sizeof(*items));
    if (items == NULL) {
        return false;
    }

    renderer->scratch_instances = items;
    renderer->scratch_instance_capacity = next_capacity;
    return true;
}

static bool renderer_prepare_batched_surface_draw(
    Renderer* renderer,
    const SpriteRenderable* item,
    uint64_t now_ms,
    RendererPreparedSurfaceDraw* prepared
) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    const Surface* baked_body;
    uint32_t frame_index;
    float width;
    float height;
    Vec2 screen_size;
    Vec2 screen;
    Rect dest;

    if (renderer == NULL || item == NULL || prepared == NULL) {
        return false;
    }

    memset(prepared, 0, sizeof(*prepared));

    source = resource_manager_get_visual_source(renderer->resource_manager, item->visual_source_handle);
    material = resource_manager_get_material(renderer->resource_manager, item->material_handle);
    if (source == NULL || material == NULL || source->draw_body == NULL) {
        return false;
    }
    if (source->draw_overlay != NULL || source->bake_policy == BAKE_POLICY_DISABLED) {
        return false;
    }
    if (fabsf(item->anchor_x - 0.5f) > 0.001f || fabsf(item->anchor_y - 0.5f) > 0.001f) {
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
    if (baked_body == NULL) {
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

static void renderer_flush_surface_batch(
    Renderer* renderer,
    RenderBackend* backend,
    RendererSurfaceBatch* batch
) {
    double started_ms;

    if (renderer == NULL || backend == NULL || batch == NULL) {
        return;
    }

    if (batch->surface == NULL || batch->count == 0U) {
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
    float width;
    float height;
    Vec2 screen_size;
    double started_ms;

    if (renderer == NULL || backend == NULL || item == NULL || !item->visible) {
        return;
    }

    source = resource_manager_get_visual_source(renderer->resource_manager, item->visual_source_handle);
    material = resource_manager_get_material(renderer->resource_manager, item->material_handle);
    shader = resource_manager_get_shader(renderer->resource_manager, item->shader_handle);
    if (source == NULL || material == NULL || source->draw_body == NULL) {
        return;
    }

    width = (float)source->width;
    height = (float)source->height;
    screen_size = camera_world_size_to_screen(&renderer->camera, (Vec2){ width, height });
    screen = camera_world_to_screen(&renderer->camera, (Vec2){ item->x, item->y });
    dest.x = screen.x - screen_size.x * item->anchor_x;
    dest.y = screen.y - screen_size.y * item->anchor_y;
    dest.w = screen_size.x;
    dest.h = screen_size.y;
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
            renderer->resource_manager,
            item->visual_source_handle,
            item->material_handle,
            item->shader_handle,
            context.frame_index,
            BAKED_SURFACE_PASS_BODY,
            item->user_data
        );
        baked_overlay = NULL;
        if (source->draw_overlay != NULL) {
            baked_overlay = resource_manager_get_baked_surface(
                renderer->resource_manager,
                item->visual_source_handle,
                item->material_handle,
                item->shader_handle,
                context.frame_index,
                BAKED_SURFACE_PASS_OVERLAY,
                item->user_data
            );
        }
        if (baked_body != NULL && (source->draw_overlay == NULL || baked_overlay != NULL)) {
            started_ms = renderer_now_ms();
            target_init(&target, backend, 0.0f, 0.0f);
            target_surface_ex(
                &target,
                baked_body,
                dest,
                (Vec2){ screen_size.x * item->anchor_x, screen_size.y * item->anchor_y },
                item->angle_radians * (180.0f / 3.14159265359f)
            );
            renderer->last_body_draw_ms += renderer_now_ms() - started_ms;
            if (source->draw_overlay != NULL) {
                started_ms = renderer_now_ms();
                target_init(&target, backend, 0.0f, 0.0f);
                target_surface_ex(
                    &target,
                    baked_overlay,
                    dest,
                    (Vec2){ screen_size.x * item->anchor_x, screen_size.y * item->anchor_y },
                    item->angle_radians * (180.0f / 3.14159265359f)
                );
                renderer->last_overlay_draw_ms += renderer_now_ms() - started_ms;
                renderer->last_overlay_draw_count += 1U;
            }
        } else {
            resource_manager_request_baked_surface(
                renderer->resource_manager,
                item->visual_source_handle,
                item->material_handle,
                item->shader_handle,
                context.frame_index,
                BAKED_SURFACE_PASS_BODY
            );
            if (source->draw_overlay != NULL) {
                resource_manager_request_baked_surface(
                    renderer->resource_manager,
                    item->visual_source_handle,
                    item->material_handle,
                    item->shader_handle,
                    context.frame_index,
                    BAKED_SURFACE_PASS_OVERLAY
                );
            }
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
    renderer->resource_manager = (ResourceManager*)calloc(1, sizeof(*renderer->resource_manager));
    renderer->debug_overlay = (DebugOverlay*)calloc(1, sizeof(*renderer->debug_overlay));
    if (renderer->resource_manager == NULL || renderer->debug_overlay == NULL) {
        free(renderer->resource_manager);
        free(renderer->debug_overlay);
        memset(renderer, 0, sizeof(*renderer));
        return;
    }
    renderer->view_width = config != NULL && config->view_width > 0 ? config->view_width : 480;
    renderer->view_height = config != NULL && config->view_height > 0 ? config->view_height : 270;
    resource_manager_init(renderer->resource_manager, backend);
    if (config != NULL && config->prebake_budget_per_frame > 0U) {
        resource_manager_set_bake_budget(renderer->resource_manager, config->prebake_budget_per_frame);
    }
    if (config != NULL) {
        resource_manager_set_bake_admission_thresholds(
            renderer->resource_manager,
            config->prebake_admission_total_hits,
            config->prebake_admission_frame_hits
        );
    }
    debug_overlay_init(renderer->debug_overlay);
    camera_init(&renderer->camera, 0.0f, 0.0f, (float)renderer->view_width, (float)renderer->view_height, 0.12f);
}

Renderer* renderer_create(RenderBackend *backend, const RendererConfig *config) {
    Renderer* renderer = (Renderer*)calloc(1, sizeof(*renderer));
    if (renderer == NULL) {
        return NULL;
    }

    renderer_init(renderer, backend, config);
    if (renderer->resource_manager == NULL || renderer->debug_overlay == NULL) {
        free(renderer);
        return NULL;
    }

    return renderer;
}

void renderer_dispose(Renderer *renderer) {
    if (renderer == NULL) {
        return;
    }
    resource_manager_dispose(renderer->resource_manager);
    debug_overlay_dispose(renderer->debug_overlay);
    free(renderer->resource_manager);
    free(renderer->debug_overlay);
    free(renderer->scratch_items);
    free(renderer->scratch_instances);
    memset(renderer, 0, sizeof(*renderer));
}

void renderer_destroy(Renderer* renderer) {
    if (renderer == NULL) {
        return;
    }

    renderer_dispose(renderer);
    free(renderer);
}

Camera* renderer_get_camera(Renderer* renderer) {
    return renderer != NULL ? &renderer->camera : NULL;
}

const Camera* renderer_get_camera_const(const Renderer* renderer) {
    return renderer != NULL ? &renderer->camera : NULL;
}

void renderer_get_viewport_size(const Renderer* renderer, int* out_width, int* out_height) {
    if (out_width != NULL) {
        *out_width = renderer != NULL ? renderer->view_width : 0;
    }
    if (out_height != NULL) {
        *out_height = renderer != NULL ? renderer->view_height : 0;
    }
}

ResourceManager* renderer_get_resource_manager(Renderer* renderer) {
    return renderer != NULL ? renderer->resource_manager : NULL;
}

const ResourceManager* renderer_get_resource_manager_const(const Renderer* renderer) {
    return renderer != NULL ? renderer->resource_manager : NULL;
}

DebugOverlay* renderer_get_debug_overlay(Renderer* renderer) {
    return renderer != NULL ? renderer->debug_overlay : NULL;
}

const DebugOverlay* renderer_get_debug_overlay_const(const Renderer* renderer) {
    return renderer != NULL ? renderer->debug_overlay : NULL;
}

size_t renderer_drain_resource_commands(Renderer* renderer, ResourceCommandQueue* queue) {
    if (renderer == NULL || renderer->resource_manager == NULL || queue == NULL) {
        return 0U;
    }

    return resource_command_queue_has_pending(queue)
        ? resource_command_queue_drain(queue, renderer->resource_manager)
        : 0U;
}

void renderer_get_stats_snapshot(const Renderer* renderer, RendererStatsSnapshot* out_snapshot) {
    if (out_snapshot == NULL) {
        return;
    }
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    if (renderer == NULL) {
        return;
    }

    out_snapshot->render_item_count = renderer->last_render_item_count;
    out_snapshot->sprite_draw_count = renderer->last_sprite_draw_count;
    out_snapshot->visible_item_count = renderer->last_visible_item_count;
    out_snapshot->overlay_draw_count = renderer->last_overlay_draw_count;
    out_snapshot->procedural_item_count = renderer->last_procedural_item_count;
    out_snapshot->baked_surface_count = renderer->last_baked_surface_count;
    out_snapshot->instanced_batch_count = renderer->last_instanced_batch_count;
    out_snapshot->instanced_draw_count = renderer->last_instanced_draw_count;
    out_snapshot->sort_ms = renderer->last_sort_ms;
    out_snapshot->visibility_ms = renderer->last_visibility_ms;
    out_snapshot->body_draw_ms = renderer->last_body_draw_ms;
    out_snapshot->overlay_draw_ms = renderer->last_overlay_draw_ms;
    out_snapshot->instance_draw_ms = renderer->last_instance_draw_ms;
}

void renderer_draw(Renderer *renderer, RenderBackend *backend, const RendererFrame *frame) {
    size_t index;
    bool needs_sort = false;
    bool batching_enabled = false;
    const SpriteRenderable* draw_items = NULL;
    uint64_t frame_signature;
    uint64_t sort_signature;
    uint64_t overlay_signature;
    uint64_t instance_signature;
    bool items_sorted_by_layer = true;
    if (renderer == NULL || backend == NULL || frame == NULL) {
        return;
    }

    renderer_compute_frame_signatures(frame, &frame_signature, &sort_signature, &instance_signature, &items_sorted_by_layer);
    overlay_signature = renderer_compute_overlay_signature(frame);
    renderer->dirty_flags = RENDERER_DIRTY_NONE;
    if (frame_signature != renderer->last_frame_signature) {
        renderer->dirty_flags |= RENDERER_DIRTY_FRAME_LIST;
    }
    if (sort_signature != renderer->last_sort_signature) {
        renderer->dirty_flags |= RENDERER_DIRTY_SORT;
    }
    if (overlay_signature != renderer->last_overlay_signature) {
        renderer->dirty_flags |= RENDERER_DIRTY_OVERLAY_LIST;
    }
    if (instance_signature != renderer->last_instance_batch_signature) {
        renderer->dirty_flags |= RENDERER_DIRTY_INSTANCE_BATCH;
    }
    if (frame->backdrop_signature != renderer->last_backdrop_signature) {
        renderer->dirty_flags |= RENDERER_DIRTY_BACKDROP;
    }
    if (frame->metadata_dirty || frame->metadata_signature != renderer->last_metadata_signature) {
        renderer->dirty_flags |= RENDERER_DIRTY_SNAPSHOT_METADATA;
    }

    renderer->last_render_item_count = frame->item_count;
    renderer->last_sprite_draw_count = 0U;
    renderer->last_visible_item_count = 0U;
    renderer->last_overlay_draw_count = 0U;
    renderer->last_procedural_item_count = 0U;
    renderer->last_baked_surface_count = resource_manager_get_baked_surface_count(renderer->resource_manager);
    renderer->last_instanced_batch_count = 0U;
    renderer->last_instanced_draw_count = 0U;
    renderer->last_sort_ms = 0.0;
    renderer->last_visibility_ms = 0.0;
    renderer->last_body_draw_ms = 0.0;
    renderer->last_overlay_draw_ms = 0.0;
    renderer->last_instance_draw_ms = 0.0;
    resource_manager_begin_frame(renderer->resource_manager);

    if (frame->draw_sprites) {
        double started_ms;
        draw_items = frame->items;
        if (frame->backdrop_draw != NULL) {
            Target target;
            target_init(&target, backend, 0.0f, 0.0f);
            frame->backdrop_draw(&target, &renderer->camera, frame->backdrop_user_data);
        }
        if (draw_items != NULL && frame->item_count == 1U) {
            started_ms = renderer_now_ms();
            if (renderer_is_item_visible(renderer, &draw_items[0])) {
                renderer->last_visible_item_count = 1U;
                renderer->last_visibility_ms = renderer_now_ms() - started_ms;
                renderer_draw_sprite_body(renderer, backend, &draw_items[0], frame->now_ms);
                renderer->last_sprite_draw_count = 1U;
            } else {
                renderer->last_visibility_ms = renderer_now_ms() - started_ms;
            }
            if (resource_manager_has_pending_bakes(renderer->resource_manager)) {
                resource_manager_process_pending_bakes(renderer->resource_manager);
            }
            goto draw_debug;
        }
        if (!items_sorted_by_layer && frame->items != NULL && frame->item_count > 0U) {
            for (index = 1U; index < frame->item_count; ++index) {
                if (frame->items[index - 1U].layer != frame->items[index].layer) {
                    needs_sort = true;
                    break;
                }
            }
        }
        if (frame->items != NULL && needs_sort && renderer_reserve_scratch(renderer, frame->item_count)) {
            started_ms = renderer_now_ms();
            memcpy(renderer->scratch_items, frame->items, frame->item_count * sizeof(*renderer->scratch_items));
            qsort(renderer->scratch_items, frame->item_count, sizeof(*renderer->scratch_items), renderer_compare_item_layer);
            renderer->last_sort_ms = renderer_now_ms() - started_ms;
            draw_items = renderer->scratch_items;
        }
        if (draw_items != NULL) {
            RendererSurfaceBatch batch = { 0 };

            batching_enabled = renderer_reserve_instances(renderer, frame->item_count);
            for (index = 0U; index < frame->item_count; ++index) {
                RendererPreparedSurfaceDraw prepared;
                double started_ms;

                started_ms = renderer_now_ms();
                if (!renderer_is_item_visible(renderer, &draw_items[index])) {
                    renderer->last_visibility_ms += renderer_now_ms() - started_ms;
                    continue;
                }
                renderer->last_visibility_ms += renderer_now_ms() - started_ms;
                renderer->last_visible_item_count += 1U;

                if (batching_enabled &&
                    renderer_prepare_batched_surface_draw(renderer, &draw_items[index], frame->now_ms, &prepared)) {
                    if (batch.surface != NULL &&
                        (prepared.surface != batch.surface || prepared.instance.tint.value != batch.tint.value)) {
                        renderer_flush_surface_batch(renderer, backend, &batch);
                    }
                    batch.surface = prepared.surface;
                    batch.tint = prepared.instance.tint;
                    renderer->scratch_instances[batch.count++] = prepared.instance;
                    continue;
                }

                renderer_flush_surface_batch(renderer, backend, &batch);
                renderer_draw_sprite_body(renderer, backend, &draw_items[index], frame->now_ms);
                renderer->last_sprite_draw_count += 1U;
            }
            renderer_flush_surface_batch(renderer, backend, &batch);
        }
        if (resource_manager_has_pending_bakes(renderer->resource_manager)) {
            resource_manager_process_pending_bakes(renderer->resource_manager);
        }
    }

draw_debug:
    if (frame->draw_debug) {
        debug_overlay_draw_world(
            renderer->debug_overlay,
            backend,
            frame->now_ms,
            &renderer->camera,
            &frame->debug_grid,
            frame->debug_entities,
            frame->debug_entity_count,
            frame->debug_collisions,
            frame->debug_collision_count,
            frame->selected_debug_entity_id,
            frame->hovered_debug_entity_id
        );
    }
    renderer->last_frame_signature = frame_signature;
    renderer->last_sort_signature = sort_signature;
    renderer->last_overlay_signature = overlay_signature;
    renderer->last_instance_batch_signature = instance_signature;
    renderer->last_backdrop_signature = frame->backdrop_signature;
    renderer->last_metadata_signature = frame->metadata_signature;
}
