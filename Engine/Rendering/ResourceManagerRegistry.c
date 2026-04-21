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
    resource_manager_mark_dirty_material(manager, resource_handle(manager->materials[index].id));
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
    resource_manager_mark_dirty_shader(manager, resource_handle(manager->shaders[index].id));
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
    manager->visual_source_values[index].bake_animation_fps = desc->bake_animation_fps;
    manager->visual_source_values[index].loop = desc->loop;
    manager->visual_source_values[index].bake_policy = desc->bake_policy;
    manager->visual_source_values[index].prebake_required = desc->prebake_required;
    manager->visual_source_values[index].bake_instance_invariant = desc->bake_instance_invariant;
    manager->visual_source_values[index].bake_ignores_material = desc->bake_ignores_material;
    manager->visual_source_values[index].bake_frame_count = desc->bake_frame_count;
    manager->visual_source_values[index].draw_body = desc->draw_body;
    manager->visual_source_values[index].draw_overlay = desc->draw_overlay;
    manager->visual_source_values[index].texture_handle = resource_handle(0U);
    if (manager->visual_source_last_requested_frame_capacity < manager->visual_source_capacity) {
        uint32_t* next_frames = (uint32_t*)realloc(
            manager->visual_source_last_requested_frame_indices,
            manager->visual_source_capacity * sizeof(*next_frames)
        );
        if (next_frames != NULL) {
            size_t fill_index;
            for (fill_index = manager->visual_source_last_requested_frame_capacity;
                 fill_index < manager->visual_source_capacity;
                 ++fill_index) {
                next_frames[fill_index] = UINT32_MAX;
            }
            manager->visual_source_last_requested_frame_indices = next_frames;
            manager->visual_source_last_requested_frame_capacity = manager->visual_source_capacity;
        }
    }
    resource_manager_mark_dirty_visual_source(manager, resource_handle(manager->visual_sources[index].id));
    if (desc->prebake_required) {
        manager->prebake_ready_visual_source_count += 1U;
    }
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
