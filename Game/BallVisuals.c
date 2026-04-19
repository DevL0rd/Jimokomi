#include "BallVisuals.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void game_draw_ball_body(
    Target *target,
    const ProceduralTextureContext *context,
    void *user_data
) {
    float time_seconds = context != NULL ? context->time_seconds : 0.0f;
    const Material* material = context != NULL ? context->material : NULL;
    float pulse_rate = material != NULL ? material->pulse_rate : 1.0f;
    float swirl_phase = time_seconds * pulse_rate;
    float pulse = 0.5f + 0.5f * sinf(swirl_phase * 1.7f);
    float orbit_radius = 2.25f + pulse * 1.25f;
    Color32 outer_color = (Color32){ material != NULL ? material->base_color : 0xf2f6ffU };
    Color32 inner_color = (Color32){ material != NULL ? material->accent_color : 0xcfd8ebU };
    Color32 glow_color = (Color32){ material != NULL ? material->glow_color : 0xffffffU };
    Color32 core_color = (Color32){ material != NULL ? 0x08101aU : 0x445066U };
    int ring_count = 4;
    int index;

    (void)user_data;

    target_circle_filled(target, (Vec2){ 7.0f, 7.0f }, 7.0f, outer_color);
    target_circle_filled(target, (Vec2){ 7.0f, 7.0f }, 5.7f, inner_color);
    target_circle_filled(target, (Vec2){ 7.0f, 7.0f }, 4.3f, core_color);
    target_circle(target, (Vec2){ 7.0f, 7.0f }, 7.0f, (Color32){ 0xe9f5ffU });

    for (index = 0; index < ring_count; ++index) {
        float ring_phase = swirl_phase * (0.8f + (float)index * 0.15f) + (float)index * 1.7f;
        float ring_radius = 1.5f + (float)index * 1.05f + sinf(ring_phase) * 0.3f;
        Vec2 ring_center = {
            14.0f + cosf(ring_phase * 0.25f) * (0.8f + (float)index * 0.35f),
            14.0f + sinf(ring_phase * 0.22f) * (0.8f + (float)index * 0.35f)
        };
        target_circle(target, ring_center, ring_radius, index % 2 == 0 ? glow_color : inner_color);
    }

    for (index = 0; index < 5; ++index) {
        float orbit_angle = swirl_phase * 0.45f + ((float)index * 1.25663706144f);
        Vec2 center = {
            7.0f + cosf(orbit_angle) * orbit_radius,
            7.0f + sinf(orbit_angle) * orbit_radius
        };
        float star_radius = 0.65f + 0.45f * (0.5f + 0.5f * sinf(swirl_phase * 2.3f + (float)index));
        target_circle_filled(target, center, star_radius, glow_color);
    }

    for (index = 0; index < 3; ++index) {
        float nebula_angle = swirl_phase + (float)index * 2.09439510239f;
        Rect nebula = {
            6.0f + cosf(nebula_angle) * 3.5f,
            7.0f + sinf(nebula_angle * 1.3f) * 2.5f,
            10.0f + pulse * 1.5f,
            4.0f + pulse
        };
        target_oval_filled(target, nebula, index == 1 ? glow_color : inner_color);
    }

    {
        float glare_pulse = 0.5f + 0.5f * sinf(time_seconds * 2.0f);
        Rect glare_fill_rect = { 5.0f, 3.0f, 10.0f + glare_pulse, 5.0f + glare_pulse * 0.8f };
        Rect glare_outline_rect = { 4.0f, 2.0f, 12.0f + glare_pulse, 7.0f + glare_pulse * 0.8f };
        target_oval_filled(target, glare_fill_rect, glow_color);
        target_oval(target, glare_outline_rect, outer_color);
        target_oval_filled(target, (Rect){ 16.0f, 17.0f, 4.0f, 2.0f }, glow_color);
    }
}

bool game_register_ball_visuals(
    Renderer* renderer,
    const RuntimeConfig* runtime_config,
    ResourceHandle* shared_shader_handle,
    ResourceHandle* source_handles,
    size_t source_count,
    ResourceHandle* material_handles,
    size_t material_count
) {
    ProceduralSourceDesc source_desc;
    Material material;
    ResourceHandle shared_source_handle;
    ResourceHandle shared_material_handle;
    size_t index;

    if (renderer == NULL || shared_shader_handle == NULL || source_handles == NULL || material_handles == NULL) {
        return false;
    }

    memset(&source_desc, 0, sizeof(source_desc));
    source_desc.width = 14;
    source_desc.height = 14;
    source_desc.animation_fps = 60.0f;
    source_desc.bake_animation_fps = 60.0f;
    source_desc.loop = true;
    source_desc.bake_policy = runtime_config != NULL
        ? runtime_config->default_procedural_bake_policy
        : BAKE_POLICY_SHARED_FRAME;
    source_desc.prebake_required = false;
    source_desc.bake_instance_invariant = runtime_config != NULL
        ? runtime_config->default_bake_instance_invariant
        : true;
    source_desc.bake_ignores_material = false;
    source_desc.bake_frame_count = 240U;
    source_desc.draw_body = game_draw_ball_body;
    source_desc.draw_overlay = NULL;
    shared_source_handle = resource_manager_register_procedural_source(
        &renderer->resource_manager,
        "procedural.space_ball.shared_60fps",
        &source_desc
    );
    if (shared_source_handle.id == 0U) {
        return false;
    }
    for (index = 0U; index < source_count; ++index) {
        source_handles[index] = shared_source_handle;
    }

    *shared_shader_handle = resource_manager_register_shader(
        &renderer->resource_manager,
        "shader.space.v1",
        SHADER_STYLE_SPACE
    );
    if (shared_shader_handle->id == 0U) {
        return false;
    }

    memset(&material, 0, sizeof(material));
    material.base_color = 0x08101aU;
    material.accent_color = 0x60a5faU;
    material.glow_color = 0xe879f9U;
    material.glare_color = 0xf8fafcU;
    material.emissive = 1.25f;
    material.distortion = 0.36f;
    material.glare_strength = 0.78f;
    material.pulse_rate = 1.15f;
    material.shader_style = SHADER_STYLE_SPACE;
    shared_material_handle = resource_manager_register_material(
        &renderer->resource_manager,
        "material.space_ball.shared",
        &material
    );
    if (shared_material_handle.id == 0U) {
        return false;
    }
    for (index = 0U; index < material_count; ++index) {
        material_handles[index] = shared_material_handle;
    }

    return true;
}

void game_draw_world_backdrop(Target* target, const Camera* camera, void* user_data) {
    const WorldBackdropConfig* config = (const WorldBackdropConfig*)user_data;
    const float tile_size = config != NULL && config->cell_size > 0.0f ? config->cell_size : 64.0f;
    const float world_width = config != NULL ? config->world_width : 1920.0f;
    const float world_height = config != NULL ? config->world_height : 1080.0f;
    int start_x;
    int end_x;
    int start_y;
    int end_y;
    int x;
    int y;

    if (target == NULL || camera == NULL) {
        return;
    }

    start_x = (int)floorf(camera->x / tile_size) - 1;
    end_x = (int)ceilf((camera->x + camera->view_width) / tile_size) + 1;
    start_y = (int)floorf(camera->y / tile_size) - 1;
    end_y = (int)ceilf((camera->y + camera->view_height) / tile_size) + 1;

    for (y = start_y; y <= end_y; ++y) {
        for (x = start_x; x <= end_x; ++x) {
            Rect cell = {
                (float)x * tile_size,
                (float)y * tile_size,
                tile_size,
                tile_size
            };
            bool even = ((x + y) & 1) == 0;
            target_rect_filled(target, cell, (Color32){ even ? 0x111827U : 0x0f172aU });
        }
    }

    target_rect(target, (Rect){ 0.0f, 0.0f, world_width, world_height }, (Color32){ 0xff4d5aU });
}
