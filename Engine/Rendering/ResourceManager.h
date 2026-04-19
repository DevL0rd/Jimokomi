#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_H

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

typedef enum BakePolicy {
    BAKE_POLICY_DISABLED = 0,
    BAKE_POLICY_SHARED_FRAME
} BakePolicy;

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

typedef struct ResourceHandle {
    uint32_t id;
} ResourceHandle;

typedef struct ProceduralSourceDesc {
    int width;
    int height;
    float animation_fps;
    bool loop;
    BakePolicy bake_policy;
    bool bake_instance_invariant;
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
    bool loop;
    BakePolicy bake_policy;
    bool bake_instance_invariant;
    uint32_t bake_frame_count;
    SpriteBuilder draw_body;
    SpriteBuilder draw_overlay;
    ResourceHandle texture_handle;
} VisualSourceResource;

typedef enum BakedSurfacePass {
    BAKED_SURFACE_PASS_BODY = 0,
    BAKED_SURFACE_PASS_OVERLAY = 1
} BakedSurfacePass;

typedef struct BakedSurfaceKey {
    uint32_t visual_source_id;
    uint32_t material_id;
    uint32_t shader_id;
    uint32_t frame_index;
    uint8_t pass;
} BakedSurfaceKey;

typedef struct BakedSurfaceResource {
    BakedSurfaceKey key;
    Surface* surface;
} BakedSurfaceResource;

typedef struct PendingBakeRequest {
    BakedSurfaceKey key;
} PendingBakeRequest;

typedef struct ResourceEntry {
    char* key;
    uint32_t id;
} ResourceEntry;

typedef struct ResourceManager {
    ResourceEntry* textures;
    TextureResource* texture_values;
    size_t texture_count;
    size_t texture_capacity;

    ResourceEntry* materials;
    MaterialResource* material_values;
    size_t material_count;
    size_t material_capacity;

    ResourceEntry* shaders;
    ShaderResource* shader_values;
    size_t shader_count;
    size_t shader_capacity;

    ResourceEntry* visual_sources;
    VisualSourceResource* visual_source_values;
    size_t visual_source_count;
    size_t visual_source_capacity;

    BakedSurfaceResource* baked_surfaces;
    size_t baked_surface_count;
    size_t baked_surface_capacity;
    PendingBakeRequest* pending_bake_requests;
    size_t pending_bake_request_count;
    size_t pending_bake_request_capacity;
    size_t pending_bake_request_head;
    size_t bake_budget_per_frame;
    size_t bake_requests_this_frame;

    RenderBackend* backend;
} ResourceManager;

bool resource_manager_init(ResourceManager* manager, RenderBackend* backend);
void resource_manager_dispose(ResourceManager* manager);
void resource_manager_begin_frame(ResourceManager* manager);

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
void resource_manager_process_pending_bakes(ResourceManager* manager);
bool resource_manager_prewarm_procedural_source(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    void* user_data
);

size_t resource_manager_get_texture_count(const ResourceManager* manager);
size_t resource_manager_get_material_count(const ResourceManager* manager);
size_t resource_manager_get_shader_count(const ResourceManager* manager);
size_t resource_manager_get_visual_source_count(const ResourceManager* manager);
size_t resource_manager_get_baked_surface_count(const ResourceManager* manager);

#endif
