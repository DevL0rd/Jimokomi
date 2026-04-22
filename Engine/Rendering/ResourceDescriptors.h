#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEDESCRIPTORS_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEDESCRIPTORS_H

#include "ResourceTypes.h"
#include "RenderTypes.h"
#include "Target.h"

#include <stdbool.h>
#include <stdint.h>

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

typedef void (*ProceduralTextureBuilder)(Target* target, const ProceduralTextureContext* context, void* user_data);

typedef struct ProceduralTextureDesc {
    GeneratedFrameConfig frames;
    int width;
    int height;
    bool bake_ignores_material;
    ProceduralTextureBuilder draw_body;
    ProceduralTextureBuilder draw_overlay;
} ProceduralTextureDesc;

typedef struct ProceduralMeshDesc {
    GeneratedFrameConfig frames;
    ProceduralMeshBuilder build_mesh;
} ProceduralMeshDesc;

typedef struct TextureResource {
    Texture* texture;
} TextureResource;

typedef struct MaterialResource {
    Material value;
} MaterialResource;

typedef struct ShaderResource {
    ShaderStyle style;
} ShaderResource;

typedef struct ProceduralTextureResource {
    GeneratedFrameConfig frames;
    int width;
    int height;
    bool bake_ignores_material;
    ProceduralTextureBuilder draw_body;
    ProceduralTextureBuilder draw_overlay;
} ProceduralTextureResource;

typedef struct ProceduralMeshResource {
    GeneratedFrameConfig frames;
    ProceduralMeshBuilder build_mesh;
} ProceduralMeshResource;

#endif
