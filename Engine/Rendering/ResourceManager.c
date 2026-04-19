#include "ResourceManager.h"

#include <stdlib.h>
#include <string.h>

static bool resource_manager_reserve(
    ResourceEntry** entries,
    void** values,
    size_t element_size,
    size_t* capacity,
    size_t required
) {
    ResourceEntry* next_entries;
    void* next_values;
    size_t next_capacity;

    if (*capacity >= required) {
        return true;
    }

    next_capacity = *capacity > 0U ? *capacity : 8U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_entries = (ResourceEntry*)realloc(*entries, next_capacity * sizeof(*next_entries));
    if (next_entries == NULL) {
        return false;
    }

    next_values = realloc(*values, next_capacity * element_size);
    if (next_values == NULL) {
        *entries = next_entries;
        return false;
    }

    *entries = next_entries;
    *values = next_values;
    *capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_baked_surfaces(
    ResourceManager* manager,
    size_t required
) {
    BakedSurfaceResource* next_values;
    size_t next_capacity;

    if (manager->baked_surface_capacity >= required) {
        return true;
    }

    next_capacity = manager->baked_surface_capacity > 0U ? manager->baked_surface_capacity : 16U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakedSurfaceResource*)realloc(
        manager->baked_surfaces,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->baked_surfaces = next_values;
    manager->baked_surface_capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_pending_bakes(
    ResourceManager* manager,
    size_t required
) {
    PendingBakeRequest* next_values;
    size_t next_capacity;

    if (manager->pending_bake_request_capacity >= required) {
        return true;
    }

    next_capacity = manager->pending_bake_request_capacity > 0U ? manager->pending_bake_request_capacity : 32U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (PendingBakeRequest*)realloc(
        manager->pending_bake_requests,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->pending_bake_requests = next_values;
    manager->pending_bake_request_capacity = next_capacity;
    return true;
}

static ResourceHandle resource_handle(uint32_t id) {
    ResourceHandle handle;
    handle.id = id;
    return handle;
}

static ResourceHandle resource_manager_find(
    const ResourceEntry* entries,
    size_t count,
    const char* key
) {
    size_t index;

    if (key == NULL) {
        return resource_handle(0U);
    }

    for (index = 0U; index < count; ++index) {
        if (entries[index].key != NULL && strcmp(entries[index].key, key) == 0) {
            return resource_handle(entries[index].id);
        }
    }

    return resource_handle(0U);
}

static void resource_manager_dispose_entries(ResourceEntry* entries, size_t count) {
    size_t index;
    for (index = 0U; index < count; ++index) {
        free(entries[index].key);
    }
}

bool resource_manager_init(ResourceManager* manager, RenderBackend* backend) {
    if (manager == NULL) {
        return false;
    }
    memset(manager, 0, sizeof(*manager));
    manager->backend = backend;
    manager->bake_budget_per_frame = 500U;
    return true;
}

void resource_manager_dispose(ResourceManager* manager) {
    if (manager == NULL) {
        return;
    }

    resource_manager_dispose_entries(manager->textures, manager->texture_count);
    resource_manager_dispose_entries(manager->materials, manager->material_count);
    resource_manager_dispose_entries(manager->shaders, manager->shader_count);
    resource_manager_dispose_entries(manager->visual_sources, manager->visual_source_count);

    if (manager->backend != NULL && manager->backend->destroy_surface != NULL) {
        size_t index;
        for (index = 0U; index < manager->baked_surface_count; ++index) {
            manager->backend->destroy_surface(manager->backend->userdata, manager->baked_surfaces[index].surface);
        }
    }

    free(manager->textures);
    free(manager->texture_values);
    free(manager->materials);
    free(manager->material_values);
    free(manager->shaders);
    free(manager->shader_values);
    free(manager->visual_sources);
    free(manager->visual_source_values);
    free(manager->baked_surfaces);
    free(manager->pending_bake_requests);
    memset(manager, 0, sizeof(*manager));
}

void resource_manager_begin_frame(ResourceManager* manager) {
    if (manager == NULL) {
        return;
    }
    manager->bake_requests_this_frame = 0U;
    if (manager->pending_bake_request_head > 0U &&
        manager->pending_bake_request_head == manager->pending_bake_request_count) {
        manager->pending_bake_request_head = 0U;
        manager->pending_bake_request_count = 0U;
    }
}

ResourceHandle resource_manager_register_texture(
    ResourceManager* manager,
    const char* key,
    Surface* surface
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || surface == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->textures, manager->texture_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->textures,
        (void**)&manager->texture_values,
        sizeof(*manager->texture_values),
        &manager->texture_capacity,
        manager->texture_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->texture_count++;
    manager->textures[index].key = string_duplicate(key);
    manager->textures[index].id = (uint32_t)(index + 1U);
    manager->texture_values[index].surface = surface;
    return resource_handle(manager->textures[index].id);
}

ResourceHandle resource_manager_register_material(
    ResourceManager* manager,
    const char* key,
    const Material* material
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || material == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->materials, manager->material_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->materials,
        (void**)&manager->material_values,
        sizeof(*manager->material_values),
        &manager->material_capacity,
        manager->material_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->material_count++;
    manager->materials[index].key = string_duplicate(key);
    manager->materials[index].id = (uint32_t)(index + 1U);
    manager->material_values[index].value = *material;
    return resource_handle(manager->materials[index].id);
}

ResourceHandle resource_manager_register_shader(
    ResourceManager* manager,
    const char* key,
    ShaderStyle style
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->shaders, manager->shader_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->shaders,
        (void**)&manager->shader_values,
        sizeof(*manager->shader_values),
        &manager->shader_capacity,
        manager->shader_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->shader_count++;
    manager->shaders[index].key = string_duplicate(key);
    manager->shaders[index].id = (uint32_t)(index + 1U);
    manager->shader_values[index].style = style;
    return resource_handle(manager->shaders[index].id);
}

ResourceHandle resource_manager_register_procedural_source(
    ResourceManager* manager,
    const char* key,
    const ProceduralSourceDesc* desc
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || desc == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->visual_sources, manager->visual_source_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->visual_sources,
        (void**)&manager->visual_source_values,
        sizeof(*manager->visual_source_values),
        &manager->visual_source_capacity,
        manager->visual_source_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->visual_source_count++;
    manager->visual_sources[index].key = string_duplicate(key);
    manager->visual_sources[index].id = (uint32_t)(index + 1U);
    manager->visual_source_values[index].kind = VISUAL_KIND_PROCEDURAL_TEXTURE;
    manager->visual_source_values[index].width = desc->width;
    manager->visual_source_values[index].height = desc->height;
    manager->visual_source_values[index].animation_fps = desc->animation_fps;
    manager->visual_source_values[index].loop = desc->loop;
    manager->visual_source_values[index].bake_policy = desc->bake_policy;
    manager->visual_source_values[index].bake_instance_invariant = desc->bake_instance_invariant;
    manager->visual_source_values[index].bake_frame_count = desc->bake_frame_count;
    manager->visual_source_values[index].draw_body = desc->draw_body;
    manager->visual_source_values[index].draw_overlay = desc->draw_overlay;
    manager->visual_source_values[index].texture_handle = resource_handle(0U);
    return resource_handle(manager->visual_sources[index].id);
}

const TextureResource* resource_manager_get_texture(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->texture_count) {
        return NULL;
    }
    return &manager->texture_values[handle.id - 1U];
}

const MaterialResource* resource_manager_get_material(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->material_count) {
        return NULL;
    }
    return &manager->material_values[handle.id - 1U];
}

const ShaderResource* resource_manager_get_shader(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->shader_count) {
        return NULL;
    }
    return &manager->shader_values[handle.id - 1U];
}

const VisualSourceResource* resource_manager_get_visual_source(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->visual_source_count) {
        return NULL;
    }
    return &manager->visual_source_values[handle.id - 1U];
}

static bool resource_manager_baked_key_equals(BakedSurfaceKey a, BakedSurfaceKey b) {
    return a.visual_source_id == b.visual_source_id &&
           a.material_id == b.material_id &&
           a.shader_id == b.shader_id &&
           a.frame_index == b.frame_index &&
           a.pass == b.pass;
}

static uint32_t resource_manager_normalize_frame_index(
    const VisualSourceResource* source,
    uint32_t frame_index
) {
    if (source == NULL) {
        return frame_index;
    }
    if (source->loop && source->bake_frame_count > 0U) {
        return frame_index % source->bake_frame_count;
    }
    return frame_index;
}

static bool resource_manager_is_bake_eligible(
    const VisualSourceResource* source,
    BakedSurfacePass pass
) {
    if (source == NULL) {
        return false;
    }
    if (source->bake_policy == BAKE_POLICY_DISABLED || !source->bake_instance_invariant) {
        return false;
    }
    if (pass == BAKED_SURFACE_PASS_BODY) {
        return source->draw_body != NULL;
    }
    if (pass == BAKED_SURFACE_PASS_OVERLAY) {
        return source->draw_overlay != NULL;
    }
    return false;
}

static const Surface* resource_manager_find_baked_surface(
    const ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t index;

    if (manager == NULL) {
        return NULL;
    }

    for (index = 0U; index < manager->baked_surface_count; ++index) {
        if (resource_manager_baked_key_equals(manager->baked_surfaces[index].key, key)) {
            return manager->baked_surfaces[index].surface;
        }
    }

    return NULL;
}

static bool resource_manager_has_pending_bake(
    const ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t index;

    if (manager == NULL) {
        return false;
    }

    for (index = manager->pending_bake_request_head; index < manager->pending_bake_request_count; ++index) {
        if (resource_manager_baked_key_equals(manager->pending_bake_requests[index].key, key)) {
            return true;
        }
    }

    return false;
}

static const Surface* resource_manager_build_baked_surface(
    ResourceManager* manager,
    const VisualSourceResource* source,
    const MaterialResource* material,
    const ShaderResource* shader,
    BakedSurfaceKey key,
    void* user_data
) {
    size_t index;
    Surface* surface;
    Target target;
    ProceduralTextureContext context;

    if (manager == NULL || source == NULL || material == NULL || manager->backend == NULL) {
        return NULL;
    }

    if (manager->bake_budget_per_frame > 0U &&
        manager->bake_requests_this_frame >= manager->bake_budget_per_frame) {
        return NULL;
    }

    if (!resource_manager_reserve_baked_surfaces(manager, manager->baked_surface_count + 1U)) {
        return NULL;
    }

    surface = manager->backend->create_surface(manager->backend->userdata, source->width, source->height);
    if (surface == NULL) {
        return NULL;
    }

    manager->bake_requests_this_frame += 1U;
    manager->backend->set_target(manager->backend->userdata, surface);
    manager->backend->clear(manager->backend->userdata, color_rgba(0U, 0U, 0U, 0U));
    target_init(&target, manager->backend, 0.0f, 0.0f);

    memset(&context, 0, sizeof(context));
    context.now_ms = (uint64_t)((source->animation_fps > 0.0f) ? ((double)key.frame_index / source->animation_fps) * 1000.0 : 0.0);
    context.time_seconds = source->animation_fps > 0.0f ? (float)key.frame_index / source->animation_fps : 0.0f;
    context.animation_fps = source->animation_fps;
    context.frame_index = key.frame_index;
    context.angle_radians = 0.0f;
    context.material = &material->value;
    context.shader_style = shader != NULL ? shader->style : material->value.shader_style;

    if (key.pass == BAKED_SURFACE_PASS_BODY) {
        source->draw_body(&target, &context, user_data);
    } else {
        source->draw_overlay(&target, &context, user_data);
    }

    manager->backend->reset_target(manager->backend->userdata);

    index = manager->baked_surface_count++;
    manager->baked_surfaces[index].key = key;
    manager->baked_surfaces[index].surface = surface;
    return surface;
}

const Surface* resource_manager_get_baked_surface(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedSurfacePass pass,
    void* user_data
) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    const Surface* baked_surface;
    BakedSurfaceKey key;

    if (manager == NULL || manager->backend == NULL ||
        manager->backend->create_surface == NULL ||
        manager->backend->set_target == NULL ||
        manager->backend->reset_target == NULL ||
        manager->backend->clear == NULL) {
        return NULL;
    }

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    material = resource_manager_get_material(manager, material_handle);
    if (source == NULL || material == NULL || !resource_manager_is_bake_eligible(source, pass)) {
        return NULL;
    }

    key.visual_source_id = visual_source_handle.id;
    key.material_id = material_handle.id;
    key.shader_id = shader_handle.id;
    key.frame_index = resource_manager_normalize_frame_index(source, frame_index);
    key.pass = (uint8_t)pass;

    baked_surface = resource_manager_find_baked_surface(manager, key);
    if (baked_surface != NULL) {
        return baked_surface;
    }

    (void)user_data;
    return NULL;
}

void resource_manager_request_baked_surface(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedSurfacePass pass
) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    BakedSurfaceKey key;

    if (manager == NULL) {
        return;
    }

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    material = resource_manager_get_material(manager, material_handle);
    if (source == NULL || material == NULL || !resource_manager_is_bake_eligible(source, pass)) {
        return;
    }

    key.visual_source_id = visual_source_handle.id;
    key.material_id = material_handle.id;
    key.shader_id = shader_handle.id;
    key.frame_index = resource_manager_normalize_frame_index(source, frame_index);
    key.pass = (uint8_t)pass;

    if (resource_manager_find_baked_surface(manager, key) != NULL ||
        resource_manager_has_pending_bake(manager, key)) {
        return;
    }

    if (!resource_manager_reserve_pending_bakes(manager, manager->pending_bake_request_count + 1U)) {
        return;
    }

    manager->pending_bake_requests[manager->pending_bake_request_count++].key = key;
}

void resource_manager_process_pending_bakes(ResourceManager* manager) {
    while (manager != NULL &&
           manager->pending_bake_request_head < manager->pending_bake_request_count &&
           (manager->bake_budget_per_frame == 0U || manager->bake_requests_this_frame < manager->bake_budget_per_frame)) {
        BakedSurfaceKey key = manager->pending_bake_requests[manager->pending_bake_request_head].key;
        const VisualSourceResource* source = resource_manager_get_visual_source(manager, resource_handle(key.visual_source_id));
        const MaterialResource* material = resource_manager_get_material(manager, resource_handle(key.material_id));
        const ShaderResource* shader = resource_manager_get_shader(manager, resource_handle(key.shader_id));

        manager->pending_bake_request_head += 1U;

        if (source == NULL || material == NULL || !resource_manager_is_bake_eligible(source, (BakedSurfacePass)key.pass)) {
            continue;
        }
        if (resource_manager_find_baked_surface(manager, key) != NULL) {
            continue;
        }

        (void)resource_manager_build_baked_surface(manager, source, material, shader, key, NULL);
    }

    if (manager != NULL &&
        manager->pending_bake_request_head > 0U &&
        manager->pending_bake_request_head == manager->pending_bake_request_count) {
        manager->pending_bake_request_head = 0U;
        manager->pending_bake_request_count = 0U;
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
    uint32_t frame_count;
    uint32_t frame_index;

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    if (!resource_manager_is_bake_eligible(source, BAKED_SURFACE_PASS_BODY) &&
        !resource_manager_is_bake_eligible(source, BAKED_SURFACE_PASS_OVERLAY)) {
        return false;
    }

    frame_count = source->bake_frame_count;
    if (frame_count == 0U) {
        frame_count = source->animation_fps > 0.0f ? (uint32_t)source->animation_fps : 1U;
    }
    if (frame_count == 0U) {
        frame_count = 1U;
    }

    for (frame_index = 0U; frame_index < frame_count; ++frame_index) {
        if (source->draw_body != NULL) {
            if (resource_manager_get_baked_surface(
                manager,
                visual_source_handle,
                material_handle,
                shader_handle,
                frame_index,
                BAKED_SURFACE_PASS_BODY,
                user_data
            ) == NULL) {
                return false;
            }
        }
        if (source->draw_overlay != NULL) {
            if (resource_manager_get_baked_surface(
                manager,
                visual_source_handle,
                material_handle,
                shader_handle,
                frame_index,
                BAKED_SURFACE_PASS_OVERLAY,
                user_data
            ) == NULL) {
                return false;
            }
        }
    }

    return true;
}

size_t resource_manager_get_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->texture_count : 0U;
}

size_t resource_manager_get_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->material_count : 0U;
}

size_t resource_manager_get_shader_count(const ResourceManager* manager) {
    return manager != NULL ? manager->shader_count : 0U;
}

size_t resource_manager_get_visual_source_count(const ResourceManager* manager) {
    return manager != NULL ? manager->visual_source_count : 0U;
}

size_t resource_manager_get_baked_surface_count(const ResourceManager* manager) {
    return manager != NULL ? manager->baked_surface_count : 0U;
}
