#include "ResourceManagerBakeCacheInternal.h"
#include "ResourceManagerBakeProcessInternal.h"
#include "ResourceManagerBakeQueueInternal.h"
#include "../Core/PlatformRuntimeInternal.h"

#include <string.h>

static double resource_manager_now_ms(void) {
    return engine_platform_now_ms();
}

const Surface* resource_manager_execute_baked_surface(
    ResourceManager* manager,
    const VisualSourceResource* source,
    const MaterialResource* material,
    const ShaderResource* shader,
    BakedSurfaceKey key,
    void* user_data
) {
    Surface* surface;
    Target target;
    ProceduralTextureContext context;
    Material neutral_material;
    const Material* context_material;

    if (manager == NULL || source == NULL || manager->backend == NULL) {
        return NULL;
    }
    if (manager->backend->create_surface == NULL ||
        manager->backend->set_target == NULL ||
        manager->backend->reset_target == NULL ||
        manager->backend->clear == NULL) {
        return NULL;
    }

    surface = manager->backend->create_surface(manager->backend->userdata, source->width, source->height);
    if (surface == NULL) {
        return NULL;
    }

    manager->bake_queue->bake_requests_this_frame += 1U;
    manager->backend->set_target(manager->backend->userdata, surface);
    manager->backend->clear(manager->backend->userdata, color_rgba(0U, 0U, 0U, 0U));
    target_init(&target, manager->backend, 0.0f, 0.0f);

    memset(&context, 0, sizeof(context));
    context.now_ms = (uint64_t)(((source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps) > 0.0f)
        ? ((double)key.frame_index / (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps)) * 1000.0
        : 0.0);
    context.time_seconds = (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps) > 0.0f
        ? (float)key.frame_index / (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps)
        : 0.0f;
    context.animation_fps = source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps;
    context.frame_index = key.frame_index;
    context.angle_radians = 0.0f;
    memset(&neutral_material, 0, sizeof(neutral_material));
    neutral_material.base_color = 0xffffffU;
    neutral_material.accent_color = 0xd0d8e8U;
    neutral_material.glow_color = 0xffffffU;
    neutral_material.glare_color = 0xffffffU;
    neutral_material.emissive = 1.0f;
    neutral_material.distortion = 0.0f;
    neutral_material.glare_strength = 1.0f;
    neutral_material.pulse_rate = 1.0f;
    neutral_material.shader_style = shader != NULL
        ? shader->style
        : (material != NULL ? material->value.shader_style : SHADER_STYLE_NONE);
    context_material = source->bake_ignores_material
        ? NULL
        : (material != NULL ? &material->value : &neutral_material);
    context.material = context_material;
    context.shader_style = shader != NULL
        ? shader->style
        : (material != NULL ? material->value.shader_style : neutral_material.shader_style);

    if (key.pass == BAKED_SURFACE_PASS_BODY) {
        source->draw_body(&target, &context, user_data);
    } else {
        source->draw_overlay(&target, &context, user_data);
    }

    manager->backend->reset_target(manager->backend->userdata);

    if (!resource_manager_store_baked_surface(manager, key, surface)) {
        if (manager->backend->destroy_surface != NULL) {
            manager->backend->destroy_surface(manager->backend->userdata, surface);
        }
        return NULL;
    }

    return surface;
}

void resource_manager_process_pending_bakes(ResourceManager* manager, double time_budget_ms) {
    double started_ms;
    double effective_budget_ms;

    if (manager == NULL) {
        return;
    }

    manager->bake_queue->last_bake_process_ms = 0.0;
    if (time_budget_ms <= 0.0) {
        return;
    }

    effective_budget_ms = time_budget_ms;
    manager->bake_queue->bake_time_budget_ms = effective_budget_ms;

    started_ms = resource_manager_now_ms();
    while ((manager->bake_queue->static_pending_bake_request_head < manager->bake_queue->static_pending_bake_request_count ||
            manager->bake_queue->transient_pending_bake_request_head < manager->bake_queue->transient_pending_bake_request_count) &&
           resource_manager_now_ms() - started_ms < effective_budget_ms) {
        PendingBakeRequest request;
        BakedSurfaceKey key;
        const VisualSourceResource* source;
        const MaterialResource* material;
        const ShaderResource* shader;

        if (manager->bake_queue->static_pending_bake_request_head < manager->bake_queue->static_pending_bake_request_count) {
            request = manager->bake_queue->static_pending_bake_requests[manager->bake_queue->static_pending_bake_request_head++];
        } else {
            request = manager->bake_queue->transient_pending_bake_requests[manager->bake_queue->transient_pending_bake_request_head++];
        }

        key = request.key;
        source = resource_manager_get_visual_source(manager, resource_handle(key.visual_source_id));
        material = resource_manager_get_material(manager, resource_handle(key.material_id));
        shader = resource_manager_get_shader(manager, resource_handle(key.shader_id));
        resource_manager_remove_pending_bake_slot(manager, key);

        if (source == NULL || !resource_manager_is_bake_eligible(source, (BakedSurfacePass)key.pass)) {
            continue;
        }
        if (!source->bake_ignores_material && material == NULL) {
            continue;
        }
        if (resource_manager_find_baked_surface(manager, key) != NULL) {
            continue;
        }

        (void)resource_manager_execute_baked_surface(manager, source, material, shader, key, NULL);
    }
    manager->bake_queue->last_bake_process_ms = resource_manager_now_ms() - started_ms;

    resource_manager_reset_empty_bake_queue(manager);
}

bool resource_manager_prewarm_procedural_source(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    void* user_data
) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    const ShaderResource* shader;
    uint32_t frame_count;
    uint32_t frame_index;
    BakedSurfaceKey key;

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    material = resource_manager_get_material(manager, material_handle);
    shader = resource_manager_get_shader(manager, shader_handle);
    if (manager == NULL || source == NULL) {
        return false;
    }
    if (!source->bake_ignores_material && material == NULL) {
        return false;
    }
    if (!resource_manager_is_bake_eligible(source, BAKED_SURFACE_PASS_BODY) &&
        !resource_manager_is_bake_eligible(source, BAKED_SURFACE_PASS_OVERLAY)) {
        return false;
    }

    frame_count = resource_manager_get_bake_frame_count(source);

    for (frame_index = 0U; frame_index < frame_count; ++frame_index) {
        if (source->draw_body != NULL) {
            key.visual_source_id = visual_source_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_SURFACE_PASS_BODY;
            if (resource_manager_find_baked_surface(manager, key) == NULL &&
                resource_manager_execute_baked_surface(manager, source, material, shader, key, user_data) == NULL) {
                return false;
            }
        }
        if (source->draw_overlay != NULL) {
            key.visual_source_id = visual_source_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_SURFACE_PASS_OVERLAY;
            if (resource_manager_find_baked_surface(manager, key) == NULL &&
                resource_manager_execute_baked_surface(manager, source, material, shader, key, user_data) == NULL) {
                return false;
            }
        }
    }

    return true;
}

size_t resource_manager_queue_required_prebake(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle
) {
    const VisualSourceResource* source;
    uint32_t frame_count;
    uint32_t frame_index;
    size_t enqueued_count = 0U;
    BakedSurfaceKey key;

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    if (manager == NULL || source == NULL || source->bake_policy == BAKE_POLICY_DISABLED || !source->prebake_required) {
        return 0U;
    }

    frame_count = resource_manager_get_bake_frame_count(source);

    for (frame_index = 0U; frame_index < frame_count; ++frame_index) {
        if (source->draw_body != NULL) {
            key.visual_source_id = visual_source_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_SURFACE_PASS_BODY;
            if (resource_manager_enqueue_baked_request(manager, key, true)) {
                enqueued_count += 1U;
            }
        }
        if (source->draw_overlay != NULL) {
            key.visual_source_id = visual_source_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_SURFACE_PASS_OVERLAY;
            if (resource_manager_enqueue_baked_request(manager, key, true)) {
                enqueued_count += 1U;
            }
        }
    }

    return enqueued_count;
}
