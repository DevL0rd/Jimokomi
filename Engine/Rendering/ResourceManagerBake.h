#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGERBAKE_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGERBAKE_H

#include "ResourceManager.h"
#include "ResourceTypes.h"
#include "RenderTypes.h"
#include "Target.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void resource_manager_set_bake_time_budget(ResourceManager* manager, double bake_time_budget_ms);
void resource_manager_set_bake_admission_thresholds(
    ResourceManager* manager,
    size_t total_hits,
    size_t frame_hits
);
const Texture* resource_manager_get_baked_texture(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedTexturePass pass,
    void* user_data
);
const Texture* resource_manager_get_or_create_baked_texture(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedTexturePass pass,
    void* user_data
);
const Mesh* resource_manager_get_or_create_baked_mesh(
    ResourceManager* manager,
    ResourceHandle procedural_mesh_handle,
    uint64_t instance_signature,
    uint32_t frame_index,
    void* user_data
);
void resource_manager_request_baked_texture(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedTexturePass pass
);
void resource_manager_request_baked_texture_for_time(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint64_t now_ms,
    BakedTexturePass pass
);
void resource_manager_process_pending_bakes(ResourceManager* manager, double time_budget_ms);
bool resource_manager_prewarm_procedural_texture(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    void* user_data
);
size_t resource_manager_queue_required_prebake(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle
);
bool resource_manager_has_pending_bakes(const ResourceManager* manager);
size_t resource_manager_get_pending_bake_count(const ResourceManager* manager);
size_t resource_manager_get_static_pending_bake_count(const ResourceManager* manager);
size_t resource_manager_get_transient_pending_bake_count(const ResourceManager* manager);

#endif
