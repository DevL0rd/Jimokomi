#include "ResourceManagerBakeQueueInternal.h"
#include "ResourceManagerInvalidationInternal.h"
#include "ResourceManagerRegistryInternal.h"
#include "ResourceManagerStatsInternal.h"

#include "RenderCommon.h"
#include "Target.h"
#include "../Core/Memory.h"

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
    Texture* texture
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || texture == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->registry->textures, manager->registry->texture_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry->textures,
        (void**)&manager->registry->texture_values,
        sizeof(*manager->registry->texture_values),
        &manager->registry->texture_capacity,
        manager->registry->texture_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry->texture_count++;
    manager->registry->textures[index].key = string_duplicate(key);
    manager->registry->textures[index].id = (uint32_t)(index + 1U);
    manager->registry->texture_values[index].texture = texture;
    return resource_handle(manager->registry->textures[index].id);
}

ResourceHandle resource_manager_register_texture_from_builder(
    ResourceManager* manager,
    const char* key,
    int width,
    int height,
    ResourceTextureBuildFn build_texture,
    void* user_data
) {
    ResourceHandle existing;
    Texture* texture;
    Target target;

    if (manager == NULL || key == NULL || width <= 0 || height <= 0 || build_texture == NULL ||
        manager->backend == NULL ||
        manager->backend->create_texture == NULL ||
        manager->backend->set_target == NULL ||
        manager->backend->reset_target == NULL ||
        manager->backend->clear == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->registry->textures, manager->registry->texture_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    texture = manager->backend->create_texture(manager->backend->userdata, width, height);
    if (texture == NULL) {
        return resource_handle(0U);
    }

    manager->backend->set_target(manager->backend->userdata, texture);
    manager->backend->clear(manager->backend->userdata, color_rgba(0U, 0U, 0U, 0U));
    target_init(&target, manager->backend, 0.0f, 0.0f);
    build_texture(&target, user_data);
    manager->backend->reset_target(manager->backend->userdata);

    existing = resource_manager_register_texture(manager, key, texture);
    if (existing.id == 0U) {
        if (manager->backend->destroy_texture != NULL) {
            manager->backend->destroy_texture(manager->backend->userdata, texture);
        }
    }
    return existing;
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

    existing = resource_manager_find(manager->registry->materials, manager->registry->material_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry->materials,
        (void**)&manager->registry->material_values,
        sizeof(*manager->registry->material_values),
        &manager->registry->material_capacity,
        manager->registry->material_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry->material_count++;
    manager->registry->materials[index].key = string_duplicate(key);
    manager->registry->materials[index].id = (uint32_t)(index + 1U);
    manager->registry->material_values[index].value = *material;
    resource_manager_mark_dirty_material(manager, resource_handle(manager->registry->materials[index].id));
    return resource_handle(manager->registry->materials[index].id);
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

    existing = resource_manager_find(manager->registry->shaders, manager->registry->shader_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry->shaders,
        (void**)&manager->registry->shader_values,
        sizeof(*manager->registry->shader_values),
        &manager->registry->shader_capacity,
        manager->registry->shader_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry->shader_count++;
    manager->registry->shaders[index].key = string_duplicate(key);
    manager->registry->shaders[index].id = (uint32_t)(index + 1U);
    manager->registry->shader_values[index].style = style;
    resource_manager_mark_dirty_shader(manager, resource_handle(manager->registry->shaders[index].id));
    return resource_handle(manager->registry->shaders[index].id);
}

ResourceHandle resource_manager_register_procedural_texture(
    ResourceManager* manager,
    const char* key,
    const ProceduralTextureDesc* desc
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || desc == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->registry->procedural_textures, manager->registry->procedural_texture_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry->procedural_textures,
        (void**)&manager->registry->procedural_texture_values,
        sizeof(*manager->registry->procedural_texture_values),
        &manager->registry->procedural_texture_capacity,
        manager->registry->procedural_texture_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry->procedural_texture_count++;
    manager->registry->procedural_textures[index].key = string_duplicate(key);
    manager->registry->procedural_textures[index].id = (uint32_t)(index + 1U);
    manager->registry->procedural_texture_values[index].frames = desc->frames;
    manager->registry->procedural_texture_values[index].width = desc->width;
    manager->registry->procedural_texture_values[index].height = desc->height;
    manager->registry->procedural_texture_values[index].bake_ignores_material = desc->bake_ignores_material;
    manager->registry->procedural_texture_values[index].draw_body = desc->draw_body;
    manager->registry->procedural_texture_values[index].draw_overlay = desc->draw_overlay;
    if (manager->bake_queue->procedural_texture_last_requested_frame_capacity < manager->registry->procedural_texture_capacity) {
        uint32_t* next_frames = (uint32_t*)realloc(
            manager->bake_queue->procedural_texture_last_requested_frame_indices,
            manager->registry->procedural_texture_capacity * sizeof(*next_frames)
        );
        if (next_frames != NULL) {
            size_t fill_index;
            for (fill_index = manager->bake_queue->procedural_texture_last_requested_frame_capacity;
                 fill_index < manager->registry->procedural_texture_capacity;
                 ++fill_index) {
                next_frames[fill_index] = UINT32_MAX;
            }
            manager->bake_queue->procedural_texture_last_requested_frame_indices = next_frames;
            manager->bake_queue->procedural_texture_last_requested_frame_capacity = manager->registry->procedural_texture_capacity;
        }
    }
    resource_manager_mark_dirty_procedural_texture(manager, resource_handle(manager->registry->procedural_textures[index].id));
    if (desc->frames.prebake_enabled &&
        desc->frames.cache_policy != BAKE_POLICY_DISABLED &&
        desc->frames.cache_policy != BAKE_POLICY_REFRESH_FRAME) {
        manager->stats->prebake_ready_procedural_texture_count += 1U;
    }
    return resource_handle(manager->registry->procedural_textures[index].id);
}

ResourceHandle resource_manager_register_procedural_mesh(
    ResourceManager* manager,
    const char* key,
    const ProceduralMeshDesc* desc
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || desc == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->registry->procedural_meshes, manager->registry->procedural_mesh_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->registry->procedural_meshes,
        (void**)&manager->registry->procedural_mesh_values,
        sizeof(*manager->registry->procedural_mesh_values),
        &manager->registry->procedural_mesh_capacity,
        manager->registry->procedural_mesh_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->registry->procedural_mesh_count++;
    manager->registry->procedural_meshes[index].key = string_duplicate(key);
    manager->registry->procedural_meshes[index].id = (uint32_t)(index + 1U);
    manager->registry->procedural_mesh_values[index].frames = desc->frames;
    manager->registry->procedural_mesh_values[index].build_mesh = desc->build_mesh;
    return resource_handle(manager->registry->procedural_meshes[index].id);
}

const TextureResource* resource_manager_get_texture(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry->texture_count) {
        return NULL;
    }
    return &manager->registry->texture_values[handle.id - 1U];
}

const MaterialResource* resource_manager_get_material(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry->material_count) {
        return NULL;
    }
    return &manager->registry->material_values[handle.id - 1U];
}

const ShaderResource* resource_manager_get_shader(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry->shader_count) {
        return NULL;
    }
    return &manager->registry->shader_values[handle.id - 1U];
}

const ProceduralTextureResource* resource_manager_get_procedural_texture(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry->procedural_texture_count) {
        return NULL;
    }
    return &manager->registry->procedural_texture_values[handle.id - 1U];
}

const ProceduralMeshResource* resource_manager_get_procedural_mesh(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->registry->procedural_mesh_count) {
        return NULL;
    }
    return &manager->registry->procedural_mesh_values[handle.id - 1U];
}
