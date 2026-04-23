#include "ResourceManagerBakeCacheInternal.h"
#include "ResourceManagerBakeProcessInternal.h"
#include "ResourceManagerBakeQueueInternal.h"
#include "GeneratedFrame.h"
#include "../Core/Profiling.h"
#include "../Core/PlatformRuntimeInternal.h"

#include <string.h>

static double resource_manager_now_ms(void) {
    return engine_platform_now_ms();
}

const Texture* resource_manager_execute_baked_texture(
    ResourceManager* manager,
    const ProceduralTextureResource* source,
    const MaterialResource* material,
    const ShaderResource* shader,
    BakedTextureKey key,
    uint32_t source_frame_index,
    void* user_data
) {
    Texture* texture;
    Target target;
    ProceduralTextureContext context;
    Material neutral_material;
    const Material* context_material;
    bool created_texture;

    if (manager == NULL || source == NULL || manager->backend == NULL) {
        return NULL;
    }
    if (manager->backend->create_texture == NULL ||
        manager->backend->set_target == NULL ||
        manager->backend->reset_target == NULL ||
        manager->backend->clear == NULL) {
        return NULL;
    }

    texture = source->frames.cache_policy == BAKE_POLICY_REFRESH_FRAME
        ? resource_manager_find_mutable_baked_texture(manager, key)
        : NULL;
    created_texture = texture == NULL;
    if (created_texture) {
        texture = manager->backend->create_texture(manager->backend->userdata, source->width, source->height);
        if (texture == NULL) {
            return NULL;
        }
    }

    manager->bake_queue->bake_requests_this_frame += 1U;
    manager->backend->set_target(manager->backend->userdata, texture);
    manager->backend->clear(manager->backend->userdata, color_rgba(0U, 0U, 0U, 0U));
    target_init(&target, manager->backend, 0.0f, 0.0f);

    memset(&context, 0, sizeof(context));
    context.time_seconds = generated_frame_cache_time_seconds(&source->frames, source_frame_index);
    context.now_ms = (uint64_t)((double)context.time_seconds * 1000.0);
    context.animation_fps = generated_frame_cache_fps(&source->frames);
    context.frame_index = source_frame_index;
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

    if (key.pass == BAKED_TEXTURE_PASS_BODY) {
        source->draw_body(&target, &context, user_data);
    } else {
        source->draw_overlay(&target, &context, user_data);
    }

    manager->backend->reset_target(manager->backend->userdata);

    if (!resource_manager_store_baked_texture(manager, key, texture, source_frame_index)) {
        if (created_texture && manager->backend->destroy_texture != NULL) {
            manager->backend->destroy_texture(manager->backend->userdata, texture);
        }
        return NULL;
    }

    return texture;
}

const Texture* resource_manager_get_or_create_baked_texture(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedTexturePass pass,
    void* user_data
) {
    const ProceduralTextureResource* source;
    const MaterialResource* material;
    const ShaderResource* shader;
    const Texture* texture;
    BakedTextureKey key;

    texture = resource_manager_get_baked_texture(
        manager,
        procedural_texture_handle,
        material_handle,
        shader_handle,
        frame_index,
        pass,
        user_data
    );
    if (texture != NULL) {
        return texture;
    }

    source = resource_manager_get_procedural_texture(manager, procedural_texture_handle);
    material = resource_manager_get_material(manager, material_handle);
    shader = resource_manager_get_shader(manager, shader_handle);
    if (manager == NULL || source == NULL || !resource_manager_is_bake_eligible(source, pass)) {
        return NULL;
    }
    if (!source->bake_ignores_material && material == NULL) {
        return NULL;
    }

    key.procedural_texture_id = procedural_texture_handle.id;
    key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
    key.shader_id = shader_handle.id;
    key.frame_index = resource_manager_normalize_frame_index(source, frame_index);
    key.pass = (uint8_t)pass;

    resource_manager_remove_pending_bake_slot(manager, key);
    return resource_manager_execute_baked_texture(
        manager,
        source,
        material,
        shader,
        key,
        generated_frame_bake_source_index(&source->frames, frame_index),
        user_data
    );
}

void resource_manager_process_pending_bakes(ResourceManager* manager, double time_budget_ms) {
    ENGINE_PROFILE_ZONE_BEGIN(process_bakes_zone, "resource_manager_process_pending_bakes");
    double started_ms;
    double effective_budget_ms;

    if (manager == NULL) {
        ENGINE_PROFILE_ZONE_END(process_bakes_zone);
        return;
    }

    manager->bake_queue->last_bake_process_ms = 0.0;
    if (time_budget_ms <= 0.0) {
        ENGINE_PROFILE_ZONE_END(process_bakes_zone);
        return;
    }

    effective_budget_ms = time_budget_ms;
    manager->bake_queue->bake_time_budget_ms = effective_budget_ms;

    started_ms = resource_manager_now_ms();
    while ((manager->bake_queue->static_pending_bake_request_head < manager->bake_queue->static_pending_bake_request_count ||
            manager->bake_queue->transient_pending_bake_request_head < manager->bake_queue->transient_pending_bake_request_count) &&
           resource_manager_now_ms() - started_ms < effective_budget_ms) {
        PendingBakeRequest request;
        BakedTextureKey key;
        const ProceduralTextureResource* source;
        const MaterialResource* material;
        const ShaderResource* shader;

        if (manager->bake_queue->static_pending_bake_request_head < manager->bake_queue->static_pending_bake_request_count) {
            request = manager->bake_queue->static_pending_bake_requests[manager->bake_queue->static_pending_bake_request_head++];
        } else {
            request = manager->bake_queue->transient_pending_bake_requests[manager->bake_queue->transient_pending_bake_request_head++];
        }

        key = request.key;
        source = resource_manager_get_procedural_texture(manager, resource_handle(key.procedural_texture_id));
        material = resource_manager_get_material(manager, resource_handle(key.material_id));
        shader = resource_manager_get_shader(manager, resource_handle(key.shader_id));
        resource_manager_remove_pending_bake_slot(manager, key);

        if (source == NULL || !resource_manager_is_bake_eligible(source, (BakedTexturePass)key.pass)) {
            continue;
        }
        if (!source->bake_ignores_material && material == NULL) {
            continue;
        }
        if (resource_manager_find_baked_texture(manager, key) != NULL) {
            continue;
        }

        (void)resource_manager_execute_baked_texture(manager, source, material, shader, key, key.frame_index, NULL);
    }
    manager->bake_queue->last_bake_process_ms = resource_manager_now_ms() - started_ms;

    resource_manager_reset_empty_bake_queue(manager);
    ENGINE_PROFILE_ZONE_END(process_bakes_zone);
}

bool resource_manager_prewarm_procedural_texture(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    void* user_data
) {
    const ProceduralTextureResource* source;
    const MaterialResource* material;
    const ShaderResource* shader;
    uint32_t frame_count;
    uint32_t frame_index;
    BakedTextureKey key;

    source = resource_manager_get_procedural_texture(manager, procedural_texture_handle);
    material = resource_manager_get_material(manager, material_handle);
    shader = resource_manager_get_shader(manager, shader_handle);
    if (manager == NULL || source == NULL) {
        return false;
    }
    if (!source->bake_ignores_material && material == NULL) {
        return false;
    }
    if (!resource_manager_is_bake_eligible(source, BAKED_TEXTURE_PASS_BODY) &&
        !resource_manager_is_bake_eligible(source, BAKED_TEXTURE_PASS_OVERLAY)) {
        return false;
    }

    frame_count = resource_manager_get_bake_frame_count(source);

    for (frame_index = 0U; frame_index < frame_count; ++frame_index) {
        if (source->draw_body != NULL) {
            key.procedural_texture_id = procedural_texture_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_TEXTURE_PASS_BODY;
            if (resource_manager_find_baked_texture(manager, key) == NULL &&
                resource_manager_execute_baked_texture(manager, source, material, shader, key, frame_index, user_data) == NULL) {
                return false;
            }
        }
        if (source->draw_overlay != NULL) {
            key.procedural_texture_id = procedural_texture_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_TEXTURE_PASS_OVERLAY;
            if (resource_manager_find_baked_texture(manager, key) == NULL &&
                resource_manager_execute_baked_texture(manager, source, material, shader, key, frame_index, user_data) == NULL) {
                return false;
            }
        }
    }

    return true;
}

size_t resource_manager_queue_required_prebake(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle
) {
    const ProceduralTextureResource* source;
    uint32_t frame_count;
    uint32_t frame_index;
    size_t enqueued_count = 0U;
    BakedTextureKey key;

    source = resource_manager_get_procedural_texture(manager, procedural_texture_handle);
    if (manager == NULL || source == NULL ||
        source->frames.cache_policy == BAKE_POLICY_DISABLED ||
        source->frames.cache_policy == BAKE_POLICY_REFRESH_FRAME ||
        !source->frames.prebake_enabled) {
        return 0U;
    }

    frame_count = resource_manager_get_bake_frame_count(source);

    for (frame_index = 0U; frame_index < frame_count; ++frame_index) {
        if (source->draw_body != NULL) {
            key.procedural_texture_id = procedural_texture_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_TEXTURE_PASS_BODY;
            if (resource_manager_enqueue_baked_request(manager, key, true)) {
                enqueued_count += 1U;
            }
        }
        if (source->draw_overlay != NULL) {
            key.procedural_texture_id = procedural_texture_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_TEXTURE_PASS_OVERLAY;
            if (resource_manager_enqueue_baked_request(manager, key, true)) {
                enqueued_count += 1U;
            }
        }
    }

    return enqueued_count;
}
