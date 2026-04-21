#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_H

#include "ResourceTypes.h"
#include "Target.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum VisualKind {
    VISUAL_KIND_TEXTURE = 0,
    VISUAL_KIND_SPRITE,
    VISUAL_KIND_PROCEDURAL_TEXTURE
} VisualKind;

typedef enum ShaderStyle {
    SHADER_STYLE_NONE = 0,
    SHADER_STYLE_UNLIT,
    SHADER_STYLE_SPACE,
    SHADER_STYLE_ADDITIVE_GLOW
} ShaderStyle;

typedef struct Material {
    uint32_t base_color;
    uint32_t accent_color;
    uint32_t glow_color;
    uint32_t glare_color;
    float emissive;
    float distortion;
    float glare_strength;
    float pulse_rate;
    ShaderStyle shader_style;
} Material;

typedef struct ProceduralTextureContext {
    uint64_t now_ms;
    float time_seconds;
    float animation_fps;
    uint32_t frame_index;
    float angle_radians;
    const Material* material;
    ShaderStyle shader_style;
} ProceduralTextureContext;

typedef void (*SpriteBuilder)(Target* target, const ProceduralTextureContext* context, void* user_data);

typedef struct ProceduralSourceDesc {
    int width;
    int height;
    float animation_fps;
    float bake_animation_fps;
    bool loop;
    BakePolicy bake_policy;
    bool prebake_required;
    bool bake_instance_invariant;
    bool bake_ignores_material;
    uint32_t bake_frame_count;
    SpriteBuilder draw_body;
    SpriteBuilder draw_overlay;
} ProceduralSourceDesc;

typedef struct TextureResource {
    Surface* surface;
} TextureResource;

typedef struct MaterialResource {
    Material value;
} MaterialResource;

typedef struct ShaderResource {
    ShaderStyle style;
} ShaderResource;

typedef struct VisualSourceResource {
    VisualKind kind;
    int width;
    int height;
    float animation_fps;
    float bake_animation_fps;
    bool loop;
    BakePolicy bake_policy;
    bool prebake_required;
    bool bake_instance_invariant;
    bool bake_ignores_material;
    uint32_t bake_frame_count;
    SpriteBuilder draw_body;
    SpriteBuilder draw_overlay;
    ResourceHandle texture_handle;
} VisualSourceResource;

typedef struct ResourceManager ResourceManager;

typedef struct ResourceManagerStatsSnapshot {
    size_t texture_count;
    size_t material_count;
    size_t shader_count;
    size_t visual_source_count;
    size_t baked_surface_count;
    size_t pending_bake_count;
    size_t static_pending_bake_count;
    size_t transient_pending_bake_count;
    size_t dirty_visual_source_count;
    size_t dirty_material_count;
    size_t dirty_shader_count;
    size_t dirty_baked_surface_count;
    size_t bake_interest_count;
    size_t bake_requests_this_frame;
    double bake_time_budget_ms;
    double bake_process_ms;
    size_t baked_surface_memory_bytes;
    uint64_t bake_cache_hits;
    uint64_t bake_cache_misses;
    uint64_t bake_invalidation_visual_source_count;
    uint64_t bake_invalidation_material_count;
    uint64_t bake_invalidation_shader_count;
    uint64_t bake_invalidation_animation_frame_count;
    uint64_t prebake_ready_visual_source_count;
    uint64_t prebake_ready_material_count;
} ResourceManagerStatsSnapshot;

bool resource_manager_init(ResourceManager* manager, RenderBackend* backend);
void resource_manager_dispose(ResourceManager* manager);
void resource_manager_begin_frame(ResourceManager* manager);
void resource_manager_set_bake_time_budget(ResourceManager* manager, double bake_time_budget_ms);
void resource_manager_set_bake_admission_thresholds(
    ResourceManager* manager,
    size_t total_hits,
    size_t frame_hits
);

ResourceHandle resource_manager_register_texture(
    ResourceManager* manager,
    const char* key,
    Surface* surface
);
ResourceHandle resource_manager_register_material(
    ResourceManager* manager,
    const char* key,
    const Material* material
);
ResourceHandle resource_manager_register_shader(
    ResourceManager* manager,
    const char* key,
    ShaderStyle style
);
ResourceHandle resource_manager_register_procedural_source(
    ResourceManager* manager,
    const char* key,
    const ProceduralSourceDesc* desc
);

const TextureResource* resource_manager_get_texture(
    const ResourceManager* manager,
    ResourceHandle handle
);
const MaterialResource* resource_manager_get_material(
    const ResourceManager* manager,
    ResourceHandle handle
);
const ShaderResource* resource_manager_get_shader(
    const ResourceManager* manager,
    ResourceHandle handle
);
const VisualSourceResource* resource_manager_get_visual_source(
    const ResourceManager* manager,
    ResourceHandle handle
);
const Surface* resource_manager_get_baked_surface(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedSurfacePass pass,
    void* user_data
);
void resource_manager_request_baked_surface(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedSurfacePass pass
);
void resource_manager_request_baked_surface_for_time(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint64_t now_ms,
    BakedSurfacePass pass
);
void resource_manager_process_pending_bakes(ResourceManager* manager, double time_budget_ms);
void resource_manager_get_stats_snapshot(const ResourceManager* manager, ResourceManagerStatsSnapshot* snapshot);
bool resource_manager_prewarm_procedural_source(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    void* user_data
);
size_t resource_manager_queue_required_prebake(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle
);
bool resource_manager_has_pending_bakes(const ResourceManager* manager);
size_t resource_manager_get_pending_bake_count(const ResourceManager* manager);
size_t resource_manager_get_static_pending_bake_count(const ResourceManager* manager);
size_t resource_manager_get_transient_pending_bake_count(const ResourceManager* manager);
size_t resource_manager_get_dirty_visual_source_count(const ResourceManager* manager);
size_t resource_manager_get_dirty_material_count(const ResourceManager* manager);
size_t resource_manager_get_dirty_shader_count(const ResourceManager* manager);
size_t resource_manager_get_dirty_baked_surface_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_cache_hit_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_cache_miss_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_invalidation_visual_source_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_invalidation_material_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_invalidation_shader_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_invalidation_animation_frame_count(const ResourceManager* manager);
uint64_t resource_manager_get_prebake_ready_visual_source_count(const ResourceManager* manager);
uint64_t resource_manager_get_prebake_ready_material_count(const ResourceManager* manager);
size_t resource_manager_get_baked_surface_memory_bytes(const ResourceManager* manager);

size_t resource_manager_get_texture_count(const ResourceManager* manager);
size_t resource_manager_get_material_count(const ResourceManager* manager);
size_t resource_manager_get_shader_count(const ResourceManager* manager);
size_t resource_manager_get_visual_source_count(const ResourceManager* manager);
size_t resource_manager_get_baked_surface_count(const ResourceManager* manager);

#endif
