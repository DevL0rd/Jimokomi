#include "LiquidVisualResources.h"

#include "../Engine/Rendering/Renderer.h"
#include "../Engine/Rendering/RendererResources.h"
#include "../Engine/Rendering/Target.h"

#include <string.h>

#define LIQUID_VISUAL_SIZE 10
#define LIQUID_VISUAL_CENTER 5.0f
#define LIQUID_VISUAL_BODY_RADIUS 4.6f

static void game_fill_liquid_material(Material* material)
{
    if (material == NULL)
    {
        return;
    }

    memset(material, 0, sizeof(*material));
    material->base_color = 0xffffffffU;
    material->accent_color = 0xffffffffU;
    material->glow_color = 0xffffffffU;
    material->glare_color = 0xffffffffU;
    material->emissive = 0.0f;
    material->distortion = 0.0f;
    material->glare_strength = 0.0f;
    material->pulse_rate = 0.0f;
    material->shader_style = SHADER_STYLE_UNLIT;
}

static void game_draw_liquid_particle_body(
    Target* target,
    const ProceduralTextureContext* context,
    void* user_data
) {
    const Material* material = context != NULL ? context->material : NULL;
    Color32 base_color = (Color32){ material != NULL ? material->base_color : 0xffffffffU };

    (void)user_data;

    target_circle_filled(
        target,
        (Vec2){ LIQUID_VISUAL_CENTER, LIQUID_VISUAL_CENTER },
        LIQUID_VISUAL_BODY_RADIUS,
        base_color
    );
}

bool game_register_liquid_visuals(Renderer* renderer, LiquidVisualResourceHandles* handles)
{
    ProceduralSourceDesc source_desc;
    Material material;

    if (renderer == NULL || handles == NULL)
    {
        return false;
    }

    memset(handles, 0, sizeof(*handles));
    memset(&source_desc, 0, sizeof(source_desc));
    source_desc.width = LIQUID_VISUAL_SIZE;
    source_desc.height = LIQUID_VISUAL_SIZE;
    source_desc.animation_fps = 10.0f;
    source_desc.bake_animation_fps = 10.0f;
    source_desc.loop = true;
    source_desc.bake_policy = RESOURCE_DEFAULT_PROCEDURAL_BAKE_POLICY;
    source_desc.prebake_required = RESOURCE_DEFAULT_PREBAKE_REQUIRED;
    source_desc.bake_instance_invariant = RESOURCE_DEFAULT_BAKE_INSTANCE_INVARIANT;
    source_desc.bake_ignores_material = true;
    source_desc.bake_frame_count = 1U;
    source_desc.draw_body = game_draw_liquid_particle_body;
    source_desc.draw_overlay = NULL;

    handles->source_handle = renderer_register_procedural_source(
        renderer,
        "procedural.liquid_particle.shared",
        &source_desc
    );
    if (handles->source_handle.id == 0U)
    {
        return false;
    }

    handles->shader_handle = renderer_register_shader(
        renderer,
        "shader.liquid_particle.unlit",
        SHADER_STYLE_UNLIT
    );
    if (handles->shader_handle.id == 0U)
    {
        return false;
    }

    game_fill_liquid_material(&material);
    handles->material_handle = renderer_register_material(
        renderer,
        "material.liquid_particle.neutral",
        &material
    );
    if (handles->material_handle.id == 0U)
    {
        return false;
    }

    return true;
}
