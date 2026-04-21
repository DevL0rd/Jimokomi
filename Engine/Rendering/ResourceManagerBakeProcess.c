#include "ResourceManagerInternal.h"

#include <string.h>
#include <time.h>

static double resource_manager_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

void resource_manager_process_pending_bakes(ResourceManager* manager, double time_budget_ms) {
    double started_ms;
    double effective_budget_ms;

    if (manager == NULL) {
        return;
    }

    effective_budget_ms = time_budget_ms > 0.0 ? time_budget_ms : manager->bake_time_budget_ms;
    manager->bake_time_budget_ms = effective_budget_ms;
    manager->last_bake_process_ms = 0.0;
    if (effective_budget_ms <= 0.0) {
        return;
    }

    started_ms = resource_manager_now_ms();
    while ((manager->static_pending_bake_request_head < manager->static_pending_bake_request_count ||
            manager->transient_pending_bake_request_head < manager->transient_pending_bake_request_count) &&
           resource_manager_now_ms() - started_ms < effective_budget_ms) {
        PendingBakeRequest request;
        BakedSurfaceKey key;
        const VisualSourceResource* source;
        const MaterialResource* material;
        const ShaderResource* shader;

        if (manager->static_pending_bake_request_head < manager->static_pending_bake_request_count) {
            request = manager->static_pending_bake_requests[manager->static_pending_bake_request_head++];
        } else {
            request = manager->transient_pending_bake_requests[manager->transient_pending_bake_request_head++];
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

        (void)resource_manager_build_baked_surface(manager, source, material, shader, key, NULL);
    }
    manager->last_bake_process_ms = resource_manager_now_ms() - started_ms;

    if (manager->static_pending_bake_request_head > 0U &&
        manager->static_pending_bake_request_head == manager->static_pending_bake_request_count) {
        manager->static_pending_bake_request_head = 0U;
        manager->static_pending_bake_request_count = 0U;
    }

    if (manager->transient_pending_bake_request_head > 0U &&
        manager->transient_pending_bake_request_head == manager->transient_pending_bake_request_count) {
        manager->transient_pending_bake_request_head = 0U;
        manager->transient_pending_bake_request_count = 0U;
    }

    if (manager->static_pending_bake_request_count == 0U &&
        manager->transient_pending_bake_request_count == 0U) {
        manager->pending_bake_slot_count = 0U;
        if (manager->pending_bake_slots != NULL && manager->pending_bake_slot_capacity > 0U) {
            memset(
                manager->pending_bake_slots,
                0,
                manager->pending_bake_slot_capacity * sizeof(*manager->pending_bake_slots)
            );
        }
    }
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
                resource_manager_build_baked_surface(manager, source, material, shader, key, user_data) == NULL) {
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
                resource_manager_build_baked_surface(manager, source, material, shader, key, user_data) == NULL) {
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
