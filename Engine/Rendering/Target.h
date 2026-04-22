#ifndef JIMOKOMI_ENGINE_RENDERING_TARGET_H
#define JIMOKOMI_ENGINE_RENDERING_TARGET_H

#include "RenderCommon.h"

typedef struct RenderBackend RenderBackend;

typedef enum RenderBlendMode {
    RENDER_BLEND_ALPHA = 0,
    RENDER_BLEND_ADDITIVE,
    RENDER_BLEND_MULTIPLY
} RenderBlendMode;

typedef struct TextureDrawInstance {
    Rect dest;
    Vec2 origin;
    float rotation_degrees;
    Color32 tint;
} TextureDrawInstance;

struct RenderBackend {
    void *userdata;
    Texture *(*create_texture)(void *userdata, int width, int height);
    void (*destroy_texture)(void *userdata, Texture *texture);
    void (*set_target)(void *userdata, Texture *texture);
    void (*reset_target)(void *userdata);
    void (*clear)(void *userdata, Color32 color);
    void (*draw_line)(void *userdata, float x0, float y0, float x1, float y1, Color32 color);
    void (*draw_rect)(void *userdata, Rect rect, Color32 color);
    void (*draw_rect_filled)(void *userdata, Rect rect, Color32 color);
    void (*draw_triangle_filled)(void *userdata, Vec2 a, Vec2 b, Vec2 c, Color32 color);
    void (*draw_circle)(void *userdata, Vec2 center, float radius, Color32 color, bool filled);
    void (*draw_oval)(void *userdata, Rect rect, Color32 color, bool filled);
    void (*draw_text)(void *userdata, float x, float y, const char *text, Color32 color);
    void (*draw_texture)(void *userdata, const Texture *texture, float x, float y);
    void (*draw_texture_tinted)(void *userdata, const Texture *texture, float x, float y, Color32 tint);
    void (*draw_texture_ex)(void *userdata, const Texture *texture, Rect dest, Vec2 origin, float rotation_degrees);
    void (*draw_texture_batch)(void *userdata, const Texture *texture, const TextureDrawInstance *instances, size_t instance_count);
    void (*draw_tilemap)(void *userdata, const void *source, int tile_x, int tile_y, float x, float y, int width_tiles, int height_tiles);
    void (*push_clip)(void *userdata, Rect rect);
    void (*pop_clip)(void *userdata);
    void (*set_blend_mode)(void *userdata, RenderBlendMode mode);
    void (*reset_blend_mode)(void *userdata);
};

typedef struct Target {
    RenderBackend *backend;
    Vec2 origin;
    Vec2 scale;
} Target;

void target_init(Target *target, RenderBackend *backend, float origin_x, float origin_y);
void target_init_scaled(Target *target, RenderBackend *backend, float origin_x, float origin_y, float scale_x, float scale_y);
void target_line(Target *target, float x0, float y0, float x1, float y1, Color32 color);
void target_rect(Target *target, Rect rect, Color32 color);
void target_rect_filled(Target *target, Rect rect, Color32 color);
void target_triangle_filled(Target *target, Vec2 a, Vec2 b, Vec2 c, Color32 color);
void target_circle(Target *target, Vec2 center, float radius, Color32 color);
void target_circle_filled(Target *target, Vec2 center, float radius, Color32 color);
void target_oval(Target *target, Rect rect, Color32 color);
void target_oval_filled(Target *target, Rect rect, Color32 color);
void target_text(Target *target, float x, float y, const char *text, Color32 color);
void target_texture(Target *target, const Texture *texture, float x, float y);
void target_texture_tinted(Target *target, const Texture *texture, float x, float y, Color32 tint);
void target_texture_ex(Target *target, const Texture *texture, Rect dest, Vec2 origin, float rotation_degrees);
void target_tilemap(Target *target, const void *source, int tile_x, int tile_y, float x, float y, int width_tiles, int height_tiles);
void target_push_clip(Target *target, Rect rect);
void target_pop_clip(Target *target);

#endif
