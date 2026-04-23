#include "ParticleVisualResources.h"

#include "RendererResources.h"
#include "ResourceDescriptors.h"
#include "Target.h"

#include <string.h>

#define PARTICLE_VISUAL_TEXTURE_SIZE 10
#define PARTICLE_VISUAL_TEXTURE_CENTER 5.0f
#define PARTICLE_VISUAL_TEXTURE_RADIUS 4.6f

static void renderer_fill_default_particle_material(
    Material* material,
    ResourceHandle texture_handle,
    ResourceHandle shader_handle
)
{
    if (material == NULL)
    {
        return;
    }

    memset(material, 0, sizeof(*material));
    material->texture_handle = texture_handle;
    material->shader_handle = shader_handle;
    material->uv_mode = MATERIAL_UV_WORLD;
    material->uv_scale_x = 96.0f;
    material->uv_scale_y = 96.0f;
    material->base_color = 0xffffffffU;
    material->accent_color = 0xffffffffU;
    material->glow_color = 0xffffffffU;
    material->glare_color = 0xffffffffU;
    material->shader_style = SHADER_STYLE_UNLIT;
}

static void renderer_fill_default_particle_mesh_material(
    Material* material,
    ResourceHandle shader_handle
)
{
    if (material == NULL)
    {
        return;
    }

    memset(material, 0, sizeof(*material));
    material->shader_handle = shader_handle;
    material->uv_mode = MATERIAL_UV_WORLD;
    material->uv_scale_x = 96.0f;
    material->uv_scale_y = 96.0f;
    material->base_color = 0xffffffffU;
    material->accent_color = 0xffffffffU;
    material->glow_color = 0xffffffffU;
    material->glare_color = 0xffffffffU;
    material->shader_style = SHADER_STYLE_UNLIT;
}

static void renderer_build_default_particle_texture(Target* target, void* user_data)
{
    (void)user_data;

    target_circle_filled(
        target,
        (Vec2){ PARTICLE_VISUAL_TEXTURE_CENTER, PARTICLE_VISUAL_TEXTURE_CENTER },
        PARTICLE_VISUAL_TEXTURE_RADIUS,
        (Color32){ 0xffffffffU }
    );
}

bool renderer_register_default_particle_visual_resources(
    Renderer* renderer,
    ParticleVisualResourceHandles* handles
)
{
    ProceduralMeshDesc mesh_desc;
    Material material;
    ResourceHandle texture_handle;
    ResourceHandle shader_handle;

    if (renderer == NULL || handles == NULL)
    {
        return false;
    }

    memset(handles, 0, sizeof(*handles));
    texture_handle = renderer_register_texture_from_builder(
        renderer,
        "texture.particle.default",
        PARTICLE_VISUAL_TEXTURE_SIZE,
        PARTICLE_VISUAL_TEXTURE_SIZE,
        renderer_build_default_particle_texture,
        NULL
    );
    if (texture_handle.id == 0U)
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
    handles->mesh_handle = renderer_register_procedural_mesh(
        renderer,
        "procedural_mesh.particle_surface.default",
        &mesh_desc
    );
    if (handles->mesh_handle.id == 0U)
    {
        return false;
    }

    shader_handle = renderer_register_shader(
        renderer,
        "shader.particle.default_unlit",
        SHADER_STYLE_UNLIT
    );
    if (shader_handle.id == 0U)
    {
        return false;
    }

    renderer_fill_default_particle_material(&material, texture_handle, shader_handle);
    handles->particle_material_handle = renderer_register_material(
        renderer,
        "material.particle.default",
        &material
    );
    if (handles->particle_material_handle.id == 0U)
    {
        return false;
    }

    renderer_fill_default_particle_mesh_material(&material, shader_handle);
    handles->mesh_material_handle = renderer_register_material(
        renderer,
        "material.particle_surface.default",
        &material
    );
    if (handles->mesh_material_handle.id == 0U)
    {
        return false;
    }

    return true;
}
