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
    float orbit_radius = 4.5f + pulse * 2.5f;
    Color32 outer_color = (Color32){ material != NULL ? material->base_color : 0xf2f6ffU };
    Color32 inner_color = (Color32){ material != NULL ? material->accent_color : 0xcfd8ebU };
    Color32 glow_color = (Color32){ material != NULL ? material->glow_color : 0xffffffU };
    Color32 core_color = (Color32){ material != NULL ? 0x08101aU : 0x445066U };
    int ring_count = 4;
    int index;

    (void)user_data;

    target_circle_filled(target, (Vec2){ 14.0f, 14.0f }, 14.0f, outer_color);
    target_circle_filled(target, (Vec2){ 14.0f, 14.0f }, 11.4f, inner_color);
    target_circle_filled(target, (Vec2){ 14.0f, 14.0f }, 8.6f, core_color);
    target_circle(target, (Vec2){ 14.0f, 14.0f }, 14.0f, (Color32){ 0xe9f5ffU });

    for (index = 0; index < ring_count; ++index) {
        float ring_phase = swirl_phase * (0.8f + (float)index * 0.15f) + (float)index * 1.7f;
        float ring_radius = 3.0f + (float)index * 2.1f + sinf(ring_phase) * 0.6f;
        Vec2 ring_center = {
            14.0f + cosf(ring_phase * 0.25f) * (0.8f + (float)index * 0.35f),
            14.0f + sinf(ring_phase * 0.22f) * (0.8f + (float)index * 0.35f)
        };
        target_circle(target, ring_center, ring_radius, index % 2 == 0 ? glow_color : inner_color);
    }

    for (index = 0; index < 5; ++index) {
        float orbit_angle = swirl_phase * 0.45f + ((float)index * 1.25663706144f);
        Vec2 center = {
            14.0f + cosf(orbit_angle) * orbit_radius,
            14.0f + sinf(orbit_angle) * orbit_radius
        };
        float star_radius = 1.3f + 0.9f * (0.5f + 0.5f * sinf(swirl_phase * 2.3f + (float)index));
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
}

static void game_draw_ball_overlay(
    Target *target,
    const ProceduralTextureContext *context,
    void *user_data
) {
    (void)user_data;
    const Material* material = context != NULL ? context->material : NULL;
    float time_seconds = context != NULL ? context->time_seconds : 0.0f;
    float pulse = 0.5f + 0.5f * sinf(time_seconds * 2.0f);
    Color32 glare_fill = (Color32){ material != NULL ? material->glare_color : 0xffffffU };
    Color32 glare_outline = (Color32){ material != NULL ? material->glow_color : 0xdbeafeU };
    target_oval_filled(target, (Rect){ 5.0f, 3.0f, 10.0f + pulse, 5.0f + pulse * 0.8f }, glare_fill);
    target_oval(target, (Rect){ 4.0f, 2.0f, 12.0f + pulse, 7.0f + pulse * 0.8f }, glare_outline);
    target_oval_filled(target, (Rect){ 16.0f, 17.0f, 4.0f, 2.0f }, glare_fill);
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
    static const uint32_t frame_options[] = { 60U, 90U, 120U, 144U, 180U, 210U, 240U, 300U };
    ProceduralSourceDesc source_desc;
    Material material;
    char key[128];
    size_t index;

    if (renderer == NULL || shared_shader_handle == NULL || source_handles == NULL || material_handles == NULL) {
        return false;
    }

    for (index = 0U; index < source_count; ++index) {
        memset(&source_desc, 0, sizeof(source_desc));
        source_desc.width = 28;
        source_desc.height = 28;
        source_desc.animation_fps = 60.0f;
        source_desc.bake_animation_fps = 15.0f;
        source_desc.loop = true;
        source_desc.bake_policy = runtime_config != NULL
            ? runtime_config->default_procedural_bake_policy
            : BAKE_POLICY_SHARED_FRAME;
        source_desc.prebake_required = false;
        source_desc.bake_instance_invariant = runtime_config != NULL
            ? runtime_config->default_bake_instance_invariant
            : true;
        source_desc.bake_ignores_material = true;
        source_desc.bake_frame_count = frame_options[index % (sizeof(frame_options) / sizeof(frame_options[0]))];
        source_desc.draw_body = game_draw_ball_body;
        source_desc.draw_overlay = game_draw_ball_overlay;
        snprintf(key, sizeof(key), "procedural.space_ball.variant_%zu", index);
        source_handles[index] = resource_manager_register_procedural_source(
            &renderer->resource_manager,
            key,
            &source_desc
        );
        if (source_handles[index].id == 0U) {
            return false;
        }
    }

    *shared_shader_handle = resource_manager_register_shader(
        &renderer->resource_manager,
        "shader.space.v1",
        SHADER_STYLE_SPACE
    );
    if (shared_shader_handle->id == 0U) {
        return false;
    }

    for (index = 0U; index < material_count; ++index) {
        uint32_t hue_step = (uint32_t)((index * 37U) & 0xffU);
        bool is_glow_variant = index != 0U && ((index % 12U) == 0U || (index % 17U) == 0U);

        memset(&material, 0, sizeof(material));
        if (index == 0U) {
            material.base_color = 0x5b21b6U;
            material.accent_color = 0xa855f7U;
            material.glow_color = 0xf472b6U;
            material.glare_color = 0xf8fafcU;
            material.emissive = 1.0f;
            material.distortion = 0.49f;
            material.glare_strength = 0.4f;
            material.pulse_rate = 1.08f;
        } else {
            material.base_color = 0x102038U + (hue_step << 8U) + ((hue_step * 3U) & 0xffU);
            material.accent_color = 0x204080U + ((hue_step * 5U) & 0xffU) + (((255U - hue_step) & 0xffU) << 8U);
            material.glow_color = 0x4060c0U + (((hue_step * 7U) & 0xffU) << 16U);
            material.glare_color = 0xf8fafcU;
            material.emissive = 0.55f + (float)(index % 7U) * 0.08f;
            material.distortion = 0.18f + (float)(index % 9U) * 0.05f;
            material.glare_strength = 0.25f + (float)(index % 5U) * 0.04f;
            material.pulse_rate = 0.45f + (float)(index % 11U) * 0.11f;

            if (is_glow_variant) {
                material.base_color = 0x09111dU;
                material.accent_color = (index % 24U) == 0U ? 0x38bdf8U : 0x22c55eU;
                material.glow_color = (index % 24U) == 0U ? 0xe0f2feU : 0xdcfce7U;
                material.glare_color = 0xffffffU;
                material.emissive = 1.35f + (float)(index % 3U) * 0.2f;
                material.distortion = 0.14f;
                material.glare_strength = 0.72f + (float)(index % 2U) * 0.14f;
                material.pulse_rate = 1.35f + (float)(index % 5U) * 0.08f;
            }
        }

        material.shader_style = SHADER_STYLE_SPACE;
        snprintf(key, sizeof(key), "material.space_ball.%zu", index);
        material_handles[index] = resource_manager_register_material(
            &renderer->resource_manager,
            key,
            &material
        );
        if (material_handles[index].id == 0U) {
            return false;
        }
    }

    return true;
}

size_t game_queue_required_ball_prebakes(
    Renderer* renderer,
    ResourceHandle shader_handle,
    const ResourceHandle* source_handles,
    size_t source_count,
    ResourceHandle representative_material_handle
) {
    size_t index;
    size_t queued = 0U;

    if (renderer == NULL || source_handles == NULL || representative_material_handle.id == 0U) {
        return 0U;
    }

    for (index = 0U; index < source_count; ++index) {
        if (source_handles[index].id == 0U) {
            continue;
        }
        queued += resource_manager_queue_required_prebake(
            &renderer->resource_manager,
            source_handles[index],
            representative_material_handle,
            shader_handle
        );
    }

    return queued;
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
