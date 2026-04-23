#include "BallVisualResources.h"

#include "GameConfig.h"
#include "../Engine/Rendering/Renderer.h"
#include "../Engine/Rendering/RendererResources.h"
#include "../Engine/Rendering/Target.h"

#include <math.h>
#include <string.h>

#define BALL_VISUAL_BASE_RADIUS (13.0f / 3.0f)
#define BALL_RENDER_SIZE ((int)ceilf(BALL_RADIUS * 2.0f))
#define BALL_RENDER_CENTER ((float)BALL_RENDER_SIZE * 0.5f)
#define BALL_RENDER_SCALE (BALL_RADIUS / BALL_VISUAL_BASE_RADIUS)
#define BALL_BODY_RADIUS BALL_RADIUS
#define BALL_WARP_LOOP_SECONDS 2.0f

static float game_ball_fract(float value)
{
    return value - floorf(value);
}

static float game_ball_hash_float(float seed)
{
    return game_ball_fract(sinf(seed * 12.9898f + 78.233f) * 43758.5453f);
}

static void game_fill_ball_material(
    Material* material,
    size_t index,
    ResourceHandle procedural_texture_handle,
    ResourceHandle shader_handle
) {
    float hue = fmodf((float)index * 37.0f, 360.0f);
    float accent_hue = fmodf(hue + 34.0f + (float)(index % 5U) * 7.0f, 360.0f);
    float glow_hue = fmodf(hue + 110.0f + (float)(index % 3U) * 11.0f, 360.0f);

    if (material == NULL) {
        return;
    }

    memset(material, 0, sizeof(*material));
    material->procedural_texture_handle = procedural_texture_handle;
    material->shader_handle = shader_handle;
    material->uv_mode = MATERIAL_UV_VERTEX;
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
    Target* target,
    const ProceduralTextureContext* context,
    void* user_data
) {
    float time_seconds = context != NULL ? context->time_seconds : 0.0f;
    float loop_t = game_ball_fract(time_seconds / BALL_WARP_LOOP_SECONDS);
    Color32 space_color = (Color32){ 0xff07111fU };
    Color32 core_color = (Color32){ 0xffffffffU };
    Color32 streak_color = (Color32){ 0xffd9ecffU };
    Color32 dim_streak_color = (Color32){ 0xff6f95c8U };
    Color32 warm_streak_color = (Color32){ 0xffffd895U };
    int index;

    (void)user_data;

    target_circle_filled(target, (Vec2){ BALL_RENDER_CENTER, BALL_RENDER_CENTER }, BALL_BODY_RADIUS, space_color);

    for (index = 0; index < 72; ++index) {
        float seed = (float)index;
        float angle = game_ball_hash_float(seed + 11.0f) * 6.28318530718f;
        float speed = 0.55f + game_ball_hash_float(seed + 23.0f) * 0.55f;
        float lane = game_ball_fract(game_ball_hash_float(seed + 37.0f) + loop_t * speed);
        float outer_distance = (0.18f + lane * 0.76f) * BALL_BODY_RADIUS;
        float length = (0.10f + lane * 0.16f) * BALL_BODY_RADIUS;
        float inner_distance = outer_distance - length;
        float outer_x;
        float outer_y;
        float inner_x;
        float inner_y;
        Color32 color;

        if (inner_distance < BALL_BODY_RADIUS * 0.08f) {
            inner_distance = BALL_BODY_RADIUS * 0.08f;
        }
        if (outer_distance > BALL_BODY_RADIUS - 2.0f) {
            outer_distance = BALL_BODY_RADIUS - 2.0f;
        }

        outer_x = BALL_RENDER_CENTER + cosf(angle) * outer_distance;
        outer_y = BALL_RENDER_CENTER + sinf(angle) * outer_distance;
        inner_x = BALL_RENDER_CENTER + cosf(angle) * inner_distance;
        inner_y = BALL_RENDER_CENTER + sinf(angle) * inner_distance;
        color = (index % 13) == 0 ? warm_streak_color : ((index % 3) == 0 ? dim_streak_color : streak_color);

        target_line(target, inner_x, inner_y, outer_x, outer_y, color);
        if ((index % 5) == 0) {
            target_circle_filled(
                target,
                (Vec2){ outer_x, outer_y },
                (0.045f + lane * 0.035f) * BALL_RENDER_SCALE,
                color
            );
        }
    }

    target_circle_filled(target, (Vec2){ BALL_RENDER_CENTER, BALL_RENDER_CENTER }, 0.42f * BALL_RENDER_SCALE, dim_streak_color);
    target_circle_filled(target, (Vec2){ BALL_RENDER_CENTER, BALL_RENDER_CENTER }, 0.22f * BALL_RENDER_SCALE, core_color);

    for (index = 0; index < 9; ++index) {
        float t = (float)index / 8.0f;
        float radius = BALL_BODY_RADIUS * (1.0f - t * 0.055f);
        uint32_t alpha = (uint32_t)(18.0f + t * 72.0f);
        target_circle(
            target,
            (Vec2){ BALL_RENDER_CENTER, BALL_RENDER_CENTER },
            radius,
            (Color32){ (alpha << 24U) }
        );
    }
}

bool game_register_ball_visuals(
    Renderer* renderer,
    ResourceHandle* material_handle
) {
    ProceduralTextureDesc texture_desc;
    Material material;
    ResourceHandle shared_procedural_texture_handle;
    ResourceHandle shared_shader_handle;
    ResourceHandle shared_material_handle;

    if (renderer == NULL || material_handle == NULL) {
        return false;
    }

    memset(&texture_desc, 0, sizeof(texture_desc));
    texture_desc.width = BALL_RENDER_SIZE;
    texture_desc.height = BALL_RENDER_SIZE;
    texture_desc.frames = (GeneratedFrameConfig){
        .animation_fps = 60.0f,
        .cache_fps = 60.0f,
        .loop = true,
        .cache_policy = RESOURCE_DEFAULT_PROCEDURAL_BAKE_POLICY,
        .prebake_enabled = RESOURCE_DEFAULT_PREBAKE_REQUIRED,
        .instance_invariant = RESOURCE_DEFAULT_BAKE_INSTANCE_INVARIANT,
        .frame_count = 240U
    };
    texture_desc.bake_ignores_material = false;
    texture_desc.draw_body = game_draw_ball_body;
    texture_desc.draw_overlay = NULL;
    shared_procedural_texture_handle = renderer_register_procedural_texture(
        renderer,
        "procedural.space_ball.shared_60fps",
        &texture_desc
    );
    if (shared_procedural_texture_handle.id == 0U) {
        return false;
    }

    shared_shader_handle = renderer_register_shader(
        renderer,
        "shader.space.v1",
        SHADER_STYLE_SPACE
    );
    if (shared_shader_handle.id == 0U) {
        return false;
    }

    game_fill_ball_material(&material, 0U, shared_procedural_texture_handle, shared_shader_handle);
    shared_material_handle = renderer_register_material(
        renderer,
        "material.space_ball.shared",
        &material
    );
    if (shared_material_handle.id == 0U) {
        return false;
    }
    *material_handle = shared_material_handle;

    return true;
}
