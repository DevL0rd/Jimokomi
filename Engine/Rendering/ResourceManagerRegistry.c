#include "ResourceManagerInternal.h"

#include "RenderCommon.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

    existing = resource_manager_find(manager->registry.textures, manager->registry.texture_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry.textures,
        (void**)&manager->registry.texture_values,
        sizeof(*manager->registry.texture_values),
        &manager->registry.texture_capacity,
        manager->registry.texture_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry.texture_count++;
    manager->registry.textures[index].key = string_duplicate(key);
    manager->registry.textures[index].id = (uint32_t)(index + 1U);
    manager->registry.texture_values[index].surface = surface;
    return resource_handle(manager->registry.textures[index].id);
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

    existing = resource_manager_find(manager->registry.materials, manager->registry.material_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry.materials,
        (void**)&manager->registry.material_values,
        sizeof(*manager->registry.material_values),
        &manager->registry.material_capacity,
        manager->registry.material_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry.material_count++;
    manager->registry.materials[index].key = string_duplicate(key);
    manager->registry.materials[index].id = (uint32_t)(index + 1U);
    manager->registry.material_values[index].value = *material;
    resource_manager_mark_dirty_material(manager, resource_handle(manager->registry.materials[index].id));
    return resource_handle(manager->registry.materials[index].id);
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

    existing = resource_manager_find(manager->registry.shaders, manager->registry.shader_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry.shaders,
        (void**)&manager->registry.shader_values,
        sizeof(*manager->registry.shader_values),
        &manager->registry.shader_capacity,
        manager->registry.shader_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry.shader_count++;
    manager->registry.shaders[index].key = string_duplicate(key);
    manager->registry.shaders[index].id = (uint32_t)(index + 1U);
    manager->registry.shader_values[index].style = style;
    resource_manager_mark_dirty_shader(manager, resource_handle(manager->registry.shaders[index].id));
    return resource_handle(manager->registry.shaders[index].id);
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

    existing = resource_manager_find(manager->registry.visual_sources, manager->registry.visual_source_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry.visual_sources,
        (void**)&manager->registry.visual_source_values,
        sizeof(*manager->registry.visual_source_values),
        &manager->registry.visual_source_capacity,
        manager->registry.visual_source_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry.visual_source_count++;
    manager->registry.visual_sources[index].key = string_duplicate(key);
    manager->registry.visual_sources[index].id = (uint32_t)(index + 1U);
    manager->registry.visual_source_values[index].kind = VISUAL_KIND_PROCEDURAL_TEXTURE;
    manager->registry.visual_source_values[index].width = desc->width;
    manager->registry.visual_source_values[index].height = desc->height;
    manager->registry.visual_source_values[index].animation_fps = desc->animation_fps;
    manager->registry.visual_source_values[index].bake_animation_fps = desc->bake_animation_fps;
    manager->registry.visual_source_values[index].loop = desc->loop;
    manager->registry.visual_source_values[index].bake_policy = desc->bake_policy;
    manager->registry.visual_source_values[index].prebake_required = desc->prebake_required;
    manager->registry.visual_source_values[index].bake_instance_invariant = desc->bake_instance_invariant;
    manager->registry.visual_source_values[index].bake_ignores_material = desc->bake_ignores_material;
    manager->registry.visual_source_values[index].bake_frame_count = desc->bake_frame_count;
    manager->registry.visual_source_values[index].draw_body = desc->draw_body;
    manager->registry.visual_source_values[index].draw_overlay = desc->draw_overlay;
    if (manager->bake_queue.visual_source_last_requested_frame_capacity < manager->registry.visual_source_capacity) {
        uint32_t* next_frames = (uint32_t*)realloc(
            manager->bake_queue.visual_source_last_requested_frame_indices,
            manager->registry.visual_source_capacity * sizeof(*next_frames)
        );
        if (next_frames != NULL) {
            size_t fill_index;
            for (fill_index = manager->bake_queue.visual_source_last_requested_frame_capacity;
                 fill_index < manager->registry.visual_source_capacity;
                 ++fill_index) {
                next_frames[fill_index] = UINT32_MAX;
            }
            manager->bake_queue.visual_source_last_requested_frame_indices = next_frames;
            manager->bake_queue.visual_source_last_requested_frame_capacity = manager->registry.visual_source_capacity;
        }
    }
    resource_manager_mark_dirty_visual_source(manager, resource_handle(manager->registry.visual_sources[index].id));
    if (desc->prebake_required) {
        manager->stats.prebake_ready_visual_source_count += 1U;
    }
    return resource_handle(manager->registry.visual_sources[index].id);
}

const TextureResource* resource_manager_get_texture(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry.texture_count) {
        return NULL;
    }
    return &manager->registry.texture_values[handle.id - 1U];
}

const MaterialResource* resource_manager_get_material(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry.material_count) {
        return NULL;
    }
    return &manager->registry.material_values[handle.id - 1U];
}

const ShaderResource* resource_manager_get_shader(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry.shader_count) {
        return NULL;
    }
    return &manager->registry.shader_values[handle.id - 1U];
}

const VisualSourceResource* resource_manager_get_visual_source(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry.visual_source_count) {
        return NULL;
    }
    return &manager->registry.visual_source_values[handle.id - 1U];
}
