#include "RendererInternal.h"

#include "DebugOverlay.h"
#include "RendererLifecycleInternal.h"
#include "RendererScratchInternal.h"
#include "RendererSignaturesInternal.h"
#include "RendererStatsInternal.h"
#include "ResourceManagerBake.h"
#include "ResourceManagerRegistry.h"
#include "ResourceManagerStats.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

double renderer_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static void renderer_draw_sprite_body(Renderer *renderer, RenderBackend *backend, const SpriteRenderable *item, uint64_t now_ms);

static bool renderer_reserve_scratch(Renderer* renderer, size_t required_capacity) {
    SpriteRenderable* items;
    size_t next_capacity;

    if (renderer == NULL) {
        return false;
    }

    if (renderer->scratch->scratch_capacity >= required_capacity) {
        return true;
    }

    next_capacity = renderer->scratch->scratch_capacity > 0U ? renderer->scratch->scratch_capacity : 16U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    items = (SpriteRenderable*)realloc(renderer->scratch->scratch_items, next_capacity * sizeof(*items));
    if (items == NULL) {
        return false;
    }

    renderer->scratch->scratch_items = items;
    renderer->scratch->scratch_capacity = next_capacity;
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
    float width;
    float height;
    Vec2 screen_size;
    double started_ms;

    if (renderer == NULL || backend == NULL || item == NULL || !item->visible) {
        return;
    }

    source = resource_manager_get_visual_source(renderer->lifecycle->resource_manager, item->visual_source_handle);
    material = resource_manager_get_material(renderer->lifecycle->resource_manager, item->material_handle);
    shader = resource_manager_get_shader(renderer->lifecycle->resource_manager, item->shader_handle);
    if (source == NULL || material == NULL || source->draw_body == NULL) {
        return;
    }

    width = (float)source->width;
    height = (float)source->height;
    screen_size = camera_world_size_to_screen(&renderer->lifecycle->camera, (Vec2){ width, height });
    screen = camera_world_to_screen(&renderer->lifecycle->camera, (Vec2){ item->x, item->y });
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
            renderer->lifecycle->resource_manager,
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
                renderer->lifecycle->resource_manager,
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
            renderer->stats->last_body_draw_ms += renderer_now_ms() - started_ms;
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
                renderer->stats->last_overlay_draw_ms += renderer_now_ms() - started_ms;
                renderer->stats->last_overlay_draw_count += 1U;
            }
        } else {
            resource_manager_request_baked_surface(
                renderer->lifecycle->resource_manager,
                item->visual_source_handle,
                item->material_handle,
                item->shader_handle,
                context.frame_index,
                BAKED_SURFACE_PASS_BODY
            );
            if (source->draw_overlay != NULL) {
                resource_manager_request_baked_surface(
                    renderer->lifecycle->resource_manager,
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
            renderer->stats->last_body_draw_ms += renderer_now_ms() - started_ms;
            if (source->draw_overlay != NULL) {
                target_init(&target, backend, dest.x, dest.y);
                started_ms = renderer_now_ms();
                source->draw_overlay(&target, &context, item->user_data);
                renderer->stats->last_overlay_draw_ms += renderer_now_ms() - started_ms;
                renderer->stats->last_overlay_draw_count += 1U;
            }
        }
    } else {
        target_init(&target, backend, dest.x, dest.y);
        started_ms = renderer_now_ms();
        source->draw_body(&target, &context, item->user_data);
        renderer->stats->last_body_draw_ms += renderer_now_ms() - started_ms;
        if (source->draw_overlay != NULL) {
            target_init(&target, backend, dest.x, dest.y);
            started_ms = renderer_now_ms();
            source->draw_overlay(&target, &context, item->user_data);
            renderer->stats->last_overlay_draw_ms += renderer_now_ms() - started_ms;
            renderer->stats->last_overlay_draw_count += 1U;
        }
    }
    if (source->kind == VISUAL_KIND_PROCEDURAL_TEXTURE) {
        renderer->stats->last_procedural_item_count += 1U;
    }
}

static int renderer_compare_item_layer(const void *left, const void *right) {
    const SpriteRenderable *a = (const SpriteRenderable *)left;
    const SpriteRenderable *b = (const SpriteRenderable *)right;
    return (a->layer > b->layer) - (a->layer < b->layer);
}

static double renderer_compute_bake_budget_ms(double frame_started_ms, float target_fps) {
    double target_frame_ms;
    double elapsed_ms;

    if (target_fps <= 0.0f) {
        target_fps = 60.0f;
    }

    target_frame_ms = 1000.0 / (double)target_fps;
    elapsed_ms = renderer_now_ms() - frame_started_ms;
    return elapsed_ms < target_frame_ms ? target_frame_ms - elapsed_ms : 0.0;
}

void renderer_init(Renderer *renderer, RenderBackend *backend, const RendererConfig *config) {
    (void)backend;
    if (renderer == NULL) {
        return;
    }
    memset(renderer, 0, sizeof(*renderer));
    renderer->lifecycle = (RendererLifecycleState*)calloc(1U, sizeof(*renderer->lifecycle));
    renderer->scratch = (RendererScratchState*)calloc(1U, sizeof(*renderer->scratch));
    renderer->stats = (RendererStatsState*)calloc(1U, sizeof(*renderer->stats));
    renderer->signatures = (RendererSignatureState*)calloc(1U, sizeof(*renderer->signatures));
    if (renderer->lifecycle == NULL || renderer->scratch == NULL || renderer->stats == NULL || renderer->signatures == NULL) {
        free(renderer->lifecycle);
        free(renderer->scratch);
        free(renderer->stats);
        free(renderer->signatures);
        memset(renderer, 0, sizeof(*renderer));
        return;
    }
    renderer->lifecycle->resource_manager = resource_manager_create(backend);
    renderer->lifecycle->debug_overlay = debug_overlay_create();
    if (renderer->lifecycle->resource_manager == NULL || renderer->lifecycle->debug_overlay == NULL) {
        resource_manager_destroy(renderer->lifecycle->resource_manager);
        debug_overlay_destroy(renderer->lifecycle->debug_overlay);
        memset(renderer, 0, sizeof(*renderer));
        return;
    }
    renderer->lifecycle->view_width = config != NULL && config->view_width > 0 ? config->view_width : 480;
    renderer->lifecycle->view_height = config != NULL && config->view_height > 0 ? config->view_height : 270;
    renderer->lifecycle->prebake_target_fps = config != NULL && config->prebake_target_fps > 0.0f
        ? config->prebake_target_fps
        : 60.0f;
    resource_manager_set_bake_time_budget(renderer->lifecycle->resource_manager, 1000.0 / (double)renderer->lifecycle->prebake_target_fps);
    if (config != NULL) {
        resource_manager_set_bake_admission_thresholds(
            renderer->lifecycle->resource_manager,
            config->prebake_admission_total_hits,
            config->prebake_admission_frame_hits
        );
    }
    camera_init(&renderer->lifecycle->camera, 0.0f, 0.0f, (float)renderer->lifecycle->view_width, (float)renderer->lifecycle->view_height, 0.12f);
}

Renderer* renderer_create(RenderBackend *backend, const RendererConfig *config) {
    Renderer* renderer = (Renderer*)calloc(1, sizeof(*renderer));
    if (renderer == NULL) {
        return NULL;
    }

    renderer_init(renderer, backend, config);
    if (renderer->lifecycle == NULL ||
        renderer->scratch == NULL ||
        renderer->stats == NULL ||
        renderer->signatures == NULL ||
        renderer->lifecycle->resource_manager == NULL ||
        renderer->lifecycle->debug_overlay == NULL) {
        renderer_dispose(renderer);
        free(renderer);
        return NULL;
    }

    return renderer;
}

void renderer_dispose(Renderer *renderer) {
    if (renderer == NULL) {
        return;
    }
    if (renderer->lifecycle != NULL) {
        resource_manager_destroy(renderer->lifecycle->resource_manager);
        debug_overlay_destroy(renderer->lifecycle->debug_overlay);
    }
    if (renderer->scratch != NULL) {
        free(renderer->scratch->scratch_items);
        free(renderer->scratch->scratch_instances);
    }
    free(renderer->lifecycle);
    free(renderer->scratch);
    free(renderer->stats);
    free(renderer->signatures);
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
    return renderer != NULL ? &renderer->lifecycle->camera : NULL;
}

const Camera* renderer_get_camera_const(const Renderer* renderer) {
    return renderer != NULL ? &renderer->lifecycle->camera : NULL;
}

void renderer_get_viewport_size(const Renderer* renderer, int* out_width, int* out_height) {
    if (out_width != NULL) {
        *out_width = renderer != NULL ? renderer->lifecycle->view_width : 0;
    }
    if (out_height != NULL) {
        *out_height = renderer != NULL ? renderer->lifecycle->view_height : 0;
    }
}

void renderer_toggle_debug_overlay(Renderer* renderer) {
    if (renderer != NULL) {
        debug_overlay_toggle(renderer->lifecycle->debug_overlay);
    }
}

void renderer_toggle_debug_world_gizmos(Renderer* renderer) {
    if (renderer != NULL) {
        debug_overlay_toggle_world_gizmos(renderer->lifecycle->debug_overlay);
    }
}

bool renderer_debug_overlay_is_ui_visible(const Renderer* renderer) {
    return renderer != NULL && debug_overlay_is_ui_visible(renderer->lifecycle->debug_overlay);
}

bool renderer_debug_overlay_is_world_gizmos_visible(const Renderer* renderer) {
    return renderer != NULL && debug_overlay_is_world_gizmos_visible(renderer->lifecycle->debug_overlay);
}

void renderer_debug_overlay_handle_input(
    Renderer* renderer,
    const EngineInput* input,
    bool has_selection,
    int window_width,
    int window_height
) {
    if (renderer != NULL) {
        debug_overlay_handle_input(renderer->lifecycle->debug_overlay, input, has_selection, window_width, window_height);
    }
}

bool renderer_debug_overlay_is_pointer_over_ui(
    const Renderer* renderer,
    bool has_selection,
    float mouse_x,
    float mouse_y,
    int window_width,
    int window_height
) {
    return renderer != NULL &&
           debug_overlay_is_pointer_over_ui(renderer->lifecycle->debug_overlay, has_selection, mouse_x, mouse_y, window_width, window_height);
}

void renderer_draw_debug_overlay_ui(
    Renderer* renderer,
    RenderBackend* backend,
    const DebugOverlaySnapshot* snapshot,
    const struct EngineStatsSnapshot* engine_stats,
    const DebugEntityView* selected_entity,
    int window_width,
    int window_height
) {
    if (renderer != NULL) {
        debug_overlay_draw_ui(renderer->lifecycle->debug_overlay, backend, snapshot, engine_stats, selected_entity, window_width, window_height);
    }
}

ResourceHandle renderer_register_material(Renderer* renderer, const char* name, const Material* material) {
    return renderer != NULL
        ? resource_manager_register_material(renderer->lifecycle->resource_manager, name, material)
        : (ResourceHandle){ 0U };
}

ResourceHandle renderer_register_shader(Renderer* renderer, const char* name, ShaderStyle style) {
    return renderer != NULL
        ? resource_manager_register_shader(renderer->lifecycle->resource_manager, name, style)
        : (ResourceHandle){ 0U };
}

ResourceHandle renderer_register_procedural_source(
    Renderer* renderer,
    const char* name,
    const ProceduralSourceDesc* desc
) {
    return renderer != NULL
        ? resource_manager_register_procedural_source(renderer->lifecycle->resource_manager, name, desc)
        : (ResourceHandle){ 0U };
}

void renderer_get_stats_snapshot(const Renderer* renderer, RendererStatsSnapshot* out_snapshot) {
    if (out_snapshot == NULL) {
        return;
    }
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    if (renderer == NULL) {
        return;
    }

    out_snapshot->render_item_count = renderer->stats->last_render_item_count;
    out_snapshot->sprite_draw_count = renderer->stats->last_sprite_draw_count;
    out_snapshot->visible_item_count = renderer->stats->last_visible_item_count;
    out_snapshot->overlay_draw_count = renderer->stats->last_overlay_draw_count;
    out_snapshot->procedural_item_count = renderer->stats->last_procedural_item_count;
    out_snapshot->baked_surface_count = renderer->stats->last_baked_surface_count;
    out_snapshot->instanced_batch_count = renderer->stats->last_instanced_batch_count;
    out_snapshot->instanced_draw_count = renderer->stats->last_instanced_draw_count;
    out_snapshot->sort_ms = renderer->stats->last_sort_ms;
    out_snapshot->visibility_ms = renderer->stats->last_visibility_ms;
    out_snapshot->body_draw_ms = renderer->stats->last_body_draw_ms;
    out_snapshot->overlay_draw_ms = renderer->stats->last_overlay_draw_ms;
    out_snapshot->instance_draw_ms = renderer->stats->last_instance_draw_ms;
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
    double frame_started_ms;
    if (renderer == NULL || backend == NULL || frame == NULL) {
        return;
    }

    frame_started_ms = renderer_now_ms();

    renderer_compute_frame_signatures(frame, &frame_signature, &sort_signature, &instance_signature, &items_sorted_by_layer);
    overlay_signature = renderer_compute_overlay_signature(frame);
    renderer->signatures->dirty_flags = RENDERER_DIRTY_NONE;
    if (frame_signature != renderer->signatures->last_frame_signature) {
        renderer->signatures->dirty_flags |= RENDERER_DIRTY_FRAME_LIST;
    }
    if (sort_signature != renderer->signatures->last_sort_signature) {
        renderer->signatures->dirty_flags |= RENDERER_DIRTY_SORT;
    }
    if (overlay_signature != renderer->signatures->last_overlay_signature) {
        renderer->signatures->dirty_flags |= RENDERER_DIRTY_OVERLAY_LIST;
    }
    if (instance_signature != renderer->signatures->last_instance_batch_signature) {
        renderer->signatures->dirty_flags |= RENDERER_DIRTY_INSTANCE_BATCH;
    }
    if (frame->backdrop_signature != renderer->signatures->last_backdrop_signature) {
        renderer->signatures->dirty_flags |= RENDERER_DIRTY_BACKDROP;
    }
    if (frame->metadata_dirty || frame->metadata_signature != renderer->signatures->last_metadata_signature) {
        renderer->signatures->dirty_flags |= RENDERER_DIRTY_SNAPSHOT_METADATA;
    }

    renderer->stats->last_render_item_count = frame->item_count;
    renderer->stats->last_sprite_draw_count = 0U;
    renderer->stats->last_visible_item_count = 0U;
    renderer->stats->last_overlay_draw_count = 0U;
    renderer->stats->last_procedural_item_count = 0U;
    renderer->stats->last_baked_surface_count = resource_manager_get_baked_surface_count(renderer->lifecycle->resource_manager);
    renderer->stats->last_instanced_batch_count = 0U;
    renderer->stats->last_instanced_draw_count = 0U;
    renderer->stats->last_sort_ms = 0.0;
    renderer->stats->last_visibility_ms = 0.0;
    renderer->stats->last_body_draw_ms = 0.0;
    renderer->stats->last_overlay_draw_ms = 0.0;
    renderer->stats->last_instance_draw_ms = 0.0;
    resource_manager_begin_frame(renderer->lifecycle->resource_manager);

    if (frame->draw_sprites) {
        double started_ms;
        draw_items = frame->items;
        if (frame->backdrop_draw != NULL) {
            Target target;
            target_init(&target, backend, 0.0f, 0.0f);
            frame->backdrop_draw(&target, &renderer->lifecycle->camera, frame->backdrop_user_data);
        }
        if (draw_items != NULL && frame->item_count == 1U) {
            started_ms = renderer_now_ms();
            if (renderer_is_item_visible(renderer, &draw_items[0])) {
                renderer->stats->last_visible_item_count = 1U;
                renderer->stats->last_visibility_ms = renderer_now_ms() - started_ms;
                renderer_draw_sprite_body(renderer, backend, &draw_items[0], frame->now_ms);
                renderer->stats->last_sprite_draw_count = 1U;
            } else {
                renderer->stats->last_visibility_ms = renderer_now_ms() - started_ms;
            }
            if (resource_manager_has_pending_bakes(renderer->lifecycle->resource_manager)) {
                resource_manager_process_pending_bakes(
                    renderer->lifecycle->resource_manager,
                    renderer_compute_bake_budget_ms(frame_started_ms, renderer->lifecycle->prebake_target_fps)
                );
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
            memcpy(renderer->scratch->scratch_items, frame->items, frame->item_count * sizeof(*renderer->scratch->scratch_items));
            qsort(renderer->scratch->scratch_items, frame->item_count, sizeof(*renderer->scratch->scratch_items), renderer_compare_item_layer);
            renderer->stats->last_sort_ms = renderer_now_ms() - started_ms;
            draw_items = renderer->scratch->scratch_items;
        }
        if (draw_items != NULL) {
            RendererSurfaceBatch batch = { 0 };

            batching_enabled = renderer_reserve_instances(renderer, frame->item_count);
            for (index = 0U; index < frame->item_count; ++index) {
                RendererPreparedSurfaceDraw prepared;
                double started_ms;

                started_ms = renderer_now_ms();
                if (!renderer_is_item_visible(renderer, &draw_items[index])) {
                    renderer->stats->last_visibility_ms += renderer_now_ms() - started_ms;
                    continue;
                }
                renderer->stats->last_visibility_ms += renderer_now_ms() - started_ms;
                renderer->stats->last_visible_item_count += 1U;

                if (batching_enabled &&
                    renderer_prepare_batched_surface_draw(renderer, &draw_items[index], frame->now_ms, &prepared)) {
                    if (batch.surface != NULL &&
                        (prepared.surface != batch.surface || prepared.instance.tint.value != batch.tint.value)) {
                        renderer_flush_surface_batch(renderer, backend, &batch);
                    }
                    batch.surface = prepared.surface;
                    batch.tint = prepared.instance.tint;
                    renderer->scratch->scratch_instances[batch.count++] = prepared.instance;
                    continue;
                }

                renderer_flush_surface_batch(renderer, backend, &batch);
                renderer_draw_sprite_body(renderer, backend, &draw_items[index], frame->now_ms);
                renderer->stats->last_sprite_draw_count += 1U;
            }
            renderer_flush_surface_batch(renderer, backend, &batch);
        }
        if (resource_manager_has_pending_bakes(renderer->lifecycle->resource_manager)) {
            resource_manager_process_pending_bakes(
                renderer->lifecycle->resource_manager,
                renderer_compute_bake_budget_ms(frame_started_ms, renderer->lifecycle->prebake_target_fps)
            );
        }
    }

draw_debug:
    if (frame->draw_debug) {
        debug_overlay_draw_world(
            renderer->lifecycle->debug_overlay,
            backend,
            frame->now_ms,
            &renderer->lifecycle->camera,
            &frame->debug_grid,
            frame->debug_entities,
            frame->debug_entity_count,
            frame->debug_collisions,
            frame->debug_collision_count,
            frame->selected_debug_entity_id,
            frame->hovered_debug_entity_id
        );
    }
    renderer->signatures->last_frame_signature = frame_signature;
    renderer->signatures->last_sort_signature = sort_signature;
    renderer->signatures->last_overlay_signature = overlay_signature;
    renderer->signatures->last_instance_batch_signature = instance_signature;
    renderer->signatures->last_backdrop_signature = frame->backdrop_signature;
    renderer->signatures->last_metadata_signature = frame->metadata_signature;
}
