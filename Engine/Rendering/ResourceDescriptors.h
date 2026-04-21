#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEDESCRIPTORS_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEDESCRIPTORS_H

#include "ResourceTypes.h"
#include "Target.h"

#include <stdbool.h>
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
} VisualSourceResource;

#endif
