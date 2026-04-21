#include "BallVisualResources.h"

#include "../Engine/Rendering/Renderer.h"
#include "../Engine/Rendering/RendererResources.h"
#include "../Engine/Rendering/Target.h"

#include <math.h>
#include <string.h>

#define BALL_RENDER_SIZE 9.0f
#define BALL_RENDER_CENTER 4.5f
#define BALL_RENDER_SCALE (2.0f / 3.0f)
#define BALL_BODY_RADIUS (13.0f / 3.0f)

static void game_fill_ball_material(Material* material, size_t index) {
    float hue = fmodf((float)index * 37.0f, 360.0f);
    float accent_hue = fmodf(hue + 34.0f + (float)(index % 5U) * 7.0f, 360.0f);
    float glow_hue = fmodf(hue + 110.0f + (float)(index % 3U) * 11.0f, 360.0f);

    if (material == NULL) {
        return;
    }

    memset(material, 0, sizeof(*material));
    material->base_color = color_rgb_from_hsv(hue, 0.68f, 0.14f + (float)(index % 5U) * 0.03f).value;
    material->accent_color = color_rgb_from_hsv(accent_hue, 0.62f, 0.75f + (float)(index % 4U) * 0.05f).value;
    material->glow_color = color_rgb_from_hsv(glow_hue, 0.35f, 0.92f).value;
    material->glare_color = color_rgb_from_hsv(glow_hue, 0.08f, 0.98f).value;
    material->emissive = 0.85f + (float)(index % 7U) * 0.08f;
    material->distortion = 0.18f + (float)(index % 9U) * 0.03f;
    material->glare_strength = 0.40f + (float)(index % 6U) * 0.06f;
    material->pulse_rate = 0.75f + (float)(index % 11U) * 0.09f;
    material->shader_style = SHADER_STYLE_SPACE;
}

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

    target_circle_filled(target, (Vec2){ BALL_RENDER_CENTER, BALL_RENDER_CENTER }, BALL_BODY_RADIUS, outer_color);
    target_circle_filled(target, (Vec2){ BALL_RENDER_CENTER, BALL_RENDER_CENTER }, 5.7f * BALL_RENDER_SCALE, inner_color);
    target_circle_filled(target, (Vec2){ BALL_RENDER_CENTER, BALL_RENDER_CENTER }, 4.3f * BALL_RENDER_SCALE, core_color);

    for (index = 0; index < ring_count; ++index) {
        float ring_phase = swirl_phase * (0.8f + (float)index * 0.15f) + (float)index * 1.7f;
        float ring_radius = (1.2f + (float)index * 0.85f + sinf(ring_phase) * 0.2f) * BALL_RENDER_SCALE;
        Vec2 ring_center = {
            BALL_RENDER_CENTER + cosf(ring_phase * 0.25f) * ((0.55f + (float)index * 0.25f) * BALL_RENDER_SCALE),
            BALL_RENDER_CENTER + sinf(ring_phase * 0.22f) * ((0.55f + (float)index * 0.25f) * BALL_RENDER_SCALE)
        };
        target_circle(target, ring_center, ring_radius, index % 2 == 0 ? glow_color : inner_color);
    }

    for (index = 0; index < 5; ++index) {
        float orbit_angle = swirl_phase * 0.45f + ((float)index * 1.25663706144f);
        Vec2 center = {
            BALL_RENDER_CENTER + cosf(orbit_angle) * (orbit_radius * BALL_RENDER_SCALE),
            BALL_RENDER_CENTER + sinf(orbit_angle) * (orbit_radius * BALL_RENDER_SCALE)
        };
        float star_radius = (0.65f + 0.45f * (0.5f + 0.5f * sinf(swirl_phase * 2.3f + (float)index))) * BALL_RENDER_SCALE;
        target_circle_filled(target, center, star_radius, glow_color);
    }

    for (index = 0; index < 3; ++index) {
        float nebula_angle = swirl_phase + (float)index * 2.09439510239f;
        float nebula_width = (6.2f + pulse * 0.6f) * BALL_RENDER_SCALE;
        float nebula_height = (2.1f + pulse * 0.4f) * BALL_RENDER_SCALE;
        Vec2 nebula_center = {
            BALL_RENDER_CENTER + cosf(nebula_angle) * (2.2f * BALL_RENDER_SCALE),
            BALL_RENDER_CENTER + sinf(nebula_angle * 1.3f) * (1.3f * BALL_RENDER_SCALE)
        };
        Rect nebula = {
            nebula_center.x - nebula_width * 0.5f,
            nebula_center.y - nebula_height * 0.5f,
            nebula_width,
            nebula_height
        };
        target_oval_filled(target, nebula, index == 1 ? glow_color : inner_color);
    }

    {
        float glare_pulse = 0.5f + 0.5f * sinf(time_seconds * 2.0f);
        Rect glare_fill_rect = {
            2.5f,
            1.5f,
            (10.0f + glare_pulse) * BALL_RENDER_SCALE,
            (5.0f + glare_pulse * 0.8f) * BALL_RENDER_SCALE
        };
        Rect glare_outline_rect = {
            2.0f,
            1.0f,
            (12.0f + glare_pulse) * BALL_RENDER_SCALE,
            (7.0f + glare_pulse * 0.8f) * BALL_RENDER_SCALE
        };
        target_oval_filled(target, glare_fill_rect, glow_color);
        target_oval(target, glare_outline_rect, outer_color);
        target_oval_filled(target, (Rect){ 8.0f, 8.5f, 2.0f, 1.0f }, glow_color);
    }

    target_circle(target, (Vec2){ BALL_RENDER_CENTER, BALL_RENDER_CENTER }, BALL_BODY_RADIUS, (Color32){ 0xe9f5ffU });
}

bool game_register_ball_visuals(
    Renderer* renderer,
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
    source_desc.width = (int)BALL_RENDER_SIZE;
    source_desc.height = (int)BALL_RENDER_SIZE;
    source_desc.animation_fps = 60.0f;
    source_desc.bake_animation_fps = 60.0f;
    source_desc.loop = true;
    source_desc.bake_policy = RESOURCE_DEFAULT_PROCEDURAL_BAKE_POLICY;
    source_desc.prebake_required = RESOURCE_DEFAULT_PREBAKE_REQUIRED;
    source_desc.bake_instance_invariant = RESOURCE_DEFAULT_BAKE_INSTANCE_INVARIANT;
    source_desc.bake_ignores_material = false;
    source_desc.bake_frame_count = 240U;
    source_desc.draw_body = game_draw_ball_body;
    source_desc.draw_overlay = NULL;
    shared_source_handle = renderer_register_procedural_source(
        renderer,
        "procedural.space_ball.shared_60fps",
        &source_desc
    );
    if (shared_source_handle.id == 0U) {
        return false;
    }
    for (index = 0U; index < source_count; ++index) {
        source_handles[index] = shared_source_handle;
    }

    *shared_shader_handle = renderer_register_shader(
        renderer,
        "shader.space.v1",
        SHADER_STYLE_SPACE
    );
    if (shared_shader_handle->id == 0U) {
        return false;
    }

    game_fill_ball_material(&material, 0U);
    shared_material_handle = renderer_register_material(
        renderer,
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
