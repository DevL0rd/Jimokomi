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

static void game_build_liquid_particle_texture(Target* target, void* user_data)
{
    (void)user_data;

    target_circle_filled(
        target,
        (Vec2){ LIQUID_VISUAL_CENTER, LIQUID_VISUAL_CENTER },
        LIQUID_VISUAL_BODY_RADIUS,
        (Color32){ 0xffffffffU }
    );
}

bool game_register_liquid_visuals(Renderer* renderer, LiquidVisualResourceHandles* handles)
{
    ProceduralMeshDesc mesh_desc;
    Material material;

    if (renderer == NULL || handles == NULL)
    {
        return false;
    }

    memset(handles, 0, sizeof(*handles));
    handles->texture_handle = renderer_register_texture_from_builder(
        renderer,
        "texture.liquid_particle.shared",
        LIQUID_VISUAL_SIZE,
        LIQUID_VISUAL_SIZE,
        game_build_liquid_particle_texture,
        NULL
    );
    if (handles->texture_handle.id == 0U)
    {
        return false;
    }

    memset(&mesh_desc, 0, sizeof(mesh_desc));
    mesh_desc.frames = (GeneratedFrameConfig){
        .animation_fps = 0.0f,
        .cache_fps = 0.0f,
        .loop = false,
        .cache_policy = BAKE_POLICY_DISABLED,
        .prebake_enabled = false,
        .instance_invariant = false,
        .frame_count = 0U
    };
    handles->procedural_mesh_handle = renderer_register_procedural_mesh(
        renderer,
        "procedural_mesh.liquid_surface.live",
        &mesh_desc
    );
    if (handles->procedural_mesh_handle.id == 0U)
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
