#include "RaylibBackendInternal.h"

#include "../Settings.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#define Texture RaylibNativeTexture
#define RenderTexture RaylibNativeRenderTexture
#include <raylib.h>
#undef RenderTexture
#undef Texture
#include <raymath.h>
#include <rlgl.h>

typedef struct RaylibTexture {
    Texture base;
    RenderTexture2D target;
} RaylibTexture;

static RaylibTexture *raylib_texture_from_base(Texture *texture);
static const RaylibTexture *raylib_texture_from_const_base(const Texture *texture);

static Color raylib_unpack_color(Color32 color) {
    Color unpacked;
    unpacked.r = (unsigned char)((color.value >> 16U) & 0xffU);
    unpacked.g = (unsigned char)((color.value >> 8U) & 0xffU);
    unpacked.b = (unsigned char)(color.value & 0xffU);
    if ((color.value & 0xff000000U) != 0U || color.value == 0U) {
        unpacked.a = (unsigned char)((color.value >> 24U) & 0xffU);
    } else {
        unpacked.a = 255U;
    }
    return unpacked;
}

bool raylib_backend_texture_get_dimensions(
    const Texture *texture,
    int *width,
    int *height
) {
    const RaylibTexture *raylib_texture = raylib_texture_from_const_base(texture);

    if (raylib_texture == NULL) {
        return false;
    }

    if (width != NULL) {
        *width = raylib_texture->base.width;
    }
    if (height != NULL) {
        *height = raylib_texture->base.height;
    }
    return true;
}

unsigned int raylib_backend_texture_get_texture_id(const Texture *texture) {
    const RaylibTexture *raylib_texture = raylib_texture_from_const_base(texture);

    return raylib_texture != NULL ? raylib_texture->target.texture.id : 0U;
}

void raylib_backend_draw_texture_batch_individual(
    void *userdata,
    const Texture *texture,
    const TextureDrawInstance *instances,
    size_t instance_count
) {
    const RaylibTexture *raylib_texture = raylib_texture_from_const_base(texture);
    Rectangle source;
    size_t index = 0U;

    (void)userdata;

    if (raylib_texture == NULL || instances == NULL)
    {
        return;
    }

    source.x = 0.0f;
    source.y = 0.0f;
    source.width = (float)raylib_texture->base.width;
    source.height = (float)-raylib_texture->base.height;

    for (index = 0U; index < instance_count; ++index)
    {
        Rectangle draw_dest = {
            instances[index].dest.x + instances[index].origin.x,
            instances[index].dest.y + instances[index].origin.y,
            instances[index].dest.w,
            instances[index].dest.h
        };
        Vector2 draw_origin = {
            instances[index].origin.x,
            instances[index].origin.y
        };
        DrawTexturePro(
            raylib_texture->target.texture,
            source,
            draw_dest,
            draw_origin,
            instances[index].rotation_degrees,
            raylib_unpack_color(instances[index].tint)
        );
    }
}

static RaylibBackend *raylib_backend_from_user_data(void *userdata) {
    return (RaylibBackend *)userdata;
}

static RaylibTexture *raylib_texture_from_base(Texture *texture) {
    return (RaylibTexture *)texture;
}

static const RaylibTexture *raylib_texture_from_const_base(const Texture *texture) {
    return (const RaylibTexture *)texture;
}

static void raylib_backend_apply_clip_rect(RaylibBackend *backend, Rect rect) {
    int x;
    int y;
    int width;
    int height;

    if (backend == NULL) {
        return;
    }

    x = (int)floorf(rect.x);
    y = (int)floorf(rect.y);
    width = (int)ceilf(rect.w);
    height = (int)ceilf(rect.h);
    if (width < 0) {
        width = 0;
    }
    if (height < 0) {
        height = 0;
    }
    BeginScissorMode(x, y, width, height);
}

static Rect raylib_backend_intersect_rect(Rect a, Rect b) {
    Rect result;
    float x0 = a.x > b.x ? a.x : b.x;
    float y0 = a.y > b.y ? a.y : b.y;
    float x1 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    float y1 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);

    result.x = x0;
    result.y = y0;
    result.w = x1 > x0 ? x1 - x0 : 0.0f;
    result.h = y1 > y0 ? y1 - y0 : 0.0f;
    return result;
}

static Texture *raylib_backend_create_texture(void *userdata, int width, int height) {
    RaylibBackend *backend = raylib_backend_from_user_data(userdata);
    RaylibTexture *texture;

    if (backend == NULL || width <= 0 || height <= 0) {
        return NULL;
    }

    texture = (RaylibTexture *)calloc(1U, sizeof(*texture));
    if (texture == NULL) {
        return NULL;
    }

    texture->base.width = width;
    texture->base.height = height;
    texture->base.bytes_per_pixel = 4U;
    texture->target = LoadRenderTexture(width, height);
    if (texture->target.id == 0U) {
        free(texture);
        return NULL;
    }

    SetTextureFilter(texture->target.texture, TEXTURE_FILTER_BILINEAR);
    return &texture->base;
}

static void raylib_backend_destroy_texture(void *userdata, Texture *texture) {
    RaylibTexture *raylib_texture = raylib_texture_from_base(texture);
    (void)userdata;

    if (raylib_texture == NULL) {
        return;
    }

    UnloadRenderTexture(raylib_texture->target);
    free(raylib_texture);
}

static void raylib_backend_set_target(void *userdata, Texture *texture) {
    RaylibBackend *backend = raylib_backend_from_user_data(userdata);
    RaylibTexture *raylib_texture = raylib_texture_from_base(texture);

    if (backend == NULL || raylib_texture == NULL) {
        return;
    }

    if (backend->clip_depth > 0) {
        while (backend->clip_depth > 0) {
            EndScissorMode();
            backend->clip_depth -= 1;
        }
    }
    BeginTextureMode(raylib_texture->target);
    backend->target_active = true;
    backend->target_width = raylib_texture->base.width;
    backend->target_height = raylib_texture->base.height;
}

static void raylib_backend_reset_target(void *userdata) {
    RaylibBackend *backend = raylib_backend_from_user_data(userdata);

    if (backend == NULL) {
        return;
    }

    if (backend->clip_depth > 0) {
        while (backend->clip_depth > 0) {
            EndScissorMode();
            backend->clip_depth -= 1;
        }
    }
    if (backend->target_active) {
        EndTextureMode();
        backend->target_active = false;
    }
    backend->target_width = GetRenderWidth();
    backend->target_height = GetRenderHeight();
}

static void raylib_backend_clear(void *userdata, Color32 color) {
    (void)userdata;
    ClearBackground(raylib_unpack_color(color));
}

static void raylib_backend_draw_line(void *userdata, float x0, float y0, float x1, float y1, Color32 color) {
    (void)userdata;
    DrawLineEx((Vector2){ x0, y0 }, (Vector2){ x1, y1 }, 1.0f, raylib_unpack_color(color));
}

static void raylib_backend_draw_rect(void *userdata, Rect rect, Color32 color) {
    (void)userdata;
    DrawRectangleLinesEx((Rectangle){ rect.x, rect.y, rect.w, rect.h }, 1.0f, raylib_unpack_color(color));
}

static void raylib_backend_draw_rect_filled(void *userdata, Rect rect, Color32 color) {
    (void)userdata;
    DrawRectangleRec((Rectangle){ rect.x, rect.y, rect.w, rect.h }, raylib_unpack_color(color));
}

static void raylib_backend_draw_triangle_filled(void *userdata, Vec2 a, Vec2 b, Vec2 c, Color32 color) {
    (void)userdata;
    DrawTriangle((Vector2){ a.x, a.y }, (Vector2){ b.x, b.y }, (Vector2){ c.x, c.y }, raylib_unpack_color(color));
}

static void raylib_backend_draw_circle(void *userdata, Vec2 center, float radius, Color32 color, bool filled) {
    (void)userdata;
    if (filled) {
        DrawCircleV((Vector2){ center.x, center.y }, radius, raylib_unpack_color(color));
    } else {
        DrawCircleLines((int)center.x, (int)center.y, radius, raylib_unpack_color(color));
    }
}

static void raylib_backend_draw_oval(void *userdata, Rect rect, Color32 color, bool filled) {
    Vector2 center = { rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f };
    (void)userdata;
    if (filled) {
        DrawEllipse((int)center.x, (int)center.y, rect.w * 0.5f, rect.h * 0.5f, raylib_unpack_color(color));
    } else {
        DrawEllipseLines((int)center.x, (int)center.y, rect.w * 0.5f, rect.h * 0.5f, raylib_unpack_color(color));
    }
}

static void raylib_backend_draw_text(void *userdata, float x, float y, const char *text, Color32 color) {
    (void)userdata;
    if (text == NULL) {
        return;
    }
    DrawText(text, (int)x, (int)y, 12, raylib_unpack_color(color));
}

static void raylib_backend_draw_texture(void *userdata, const Texture *texture, float x, float y) {
    const RaylibTexture *raylib_texture = raylib_texture_from_const_base(texture);
    Rectangle source;
    Rectangle dest;
    (void)userdata;

    if (raylib_texture == NULL) {
        return;
    }

    source.x = 0.0f;
    source.y = 0.0f;
    source.width = (float)raylib_texture->base.width;
    source.height = (float)-raylib_texture->base.height;
    dest.x = x;
    dest.y = y;
    dest.width = (float)raylib_texture->base.width;
    dest.height = (float)raylib_texture->base.height;
    DrawTexturePro(raylib_texture->target.texture, source, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
}

static void raylib_backend_draw_texture_tinted(void *userdata, const Texture *texture, float x, float y, Color32 tint) {
    const RaylibTexture *raylib_texture = raylib_texture_from_const_base(texture);
    Rectangle source;
    Rectangle dest;
    (void)userdata;

    if (raylib_texture == NULL) {
        return;
    }

    source.x = 0.0f;
    source.y = 0.0f;
    source.width = (float)raylib_texture->base.width;
    source.height = (float)-raylib_texture->base.height;
    dest.x = x;
    dest.y = y;
    dest.width = (float)raylib_texture->base.width;
    dest.height = (float)raylib_texture->base.height;
    DrawTexturePro(raylib_texture->target.texture, source, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, raylib_unpack_color(tint));
}

static void raylib_backend_draw_texture_ex(
    void *userdata,
    const Texture *texture,
    Rect dest,
    Vec2 origin,
    float rotation_degrees
) {
    const RaylibTexture *raylib_texture = raylib_texture_from_const_base(texture);
    Rectangle source;
    Rectangle draw_dest;
    Vector2 draw_origin;
    (void)userdata;

    if (raylib_texture == NULL) {
        return;
    }

    source.x = 0.0f;
    source.y = 0.0f;
    source.width = (float)raylib_texture->base.width;
    source.height = (float)-raylib_texture->base.height;
    draw_dest.x = dest.x + origin.x;
    draw_dest.y = dest.y + origin.y;
    draw_dest.width = dest.w;
    draw_dest.height = dest.h;
    draw_origin.x = origin.x;
    draw_origin.y = origin.y;
    DrawTexturePro(raylib_texture->target.texture, source, draw_dest, draw_origin, rotation_degrees, WHITE);
}

static void raylib_backend_draw_tilemap(
    void *userdata,
    const void *source,
    int tile_x,
    int tile_y,
    float x,
    float y,
    int width_tiles,
    int height_tiles
) {
    (void)userdata;
    (void)source;
    (void)tile_x;
    (void)tile_y;
    (void)x;
    (void)y;
    (void)width_tiles;
    (void)height_tiles;
}

static void raylib_backend_push_clip(void *userdata, Rect rect) {
    RaylibBackend *backend = raylib_backend_from_user_data(userdata);
    Rect next_rect = rect;

    if (backend == NULL || backend->clip_depth >= (int)(sizeof(backend->clip_stack) / sizeof(backend->clip_stack[0]))) {
        return;
    }

    if (backend->clip_depth > 0) {
        EndScissorMode();
        next_rect = raylib_backend_intersect_rect(backend->clip_stack[backend->clip_depth - 1], rect);
    }

    backend->clip_stack[backend->clip_depth++] = next_rect;
    raylib_backend_apply_clip_rect(backend, next_rect);
}

static void raylib_backend_pop_clip(void *userdata) {
    RaylibBackend *backend = raylib_backend_from_user_data(userdata);

    if (backend == NULL || backend->clip_depth <= 0) {
        return;
    }

    EndScissorMode();
    backend->clip_depth -= 1;
    if (backend->clip_depth > 0) {
        raylib_backend_apply_clip_rect(backend, backend->clip_stack[backend->clip_depth - 1]);
    }
}

static void raylib_backend_set_blend_mode(void *userdata, RenderBlendMode mode) {
    (void)userdata;
    switch (mode) {
        case RENDER_BLEND_ADDITIVE:
            BeginBlendMode(BLEND_ADDITIVE);
            break;
        case RENDER_BLEND_MULTIPLY:
            BeginBlendMode(BLEND_MULTIPLIED);
            break;
        case RENDER_BLEND_ALPHA:
        default:
            BeginBlendMode(BLEND_ALPHA);
            break;
    }
}

static void raylib_backend_reset_blend_mode(void *userdata) {
    (void)userdata;
    EndBlendMode();
}

bool raylib_backend_init(
    RaylibBackend *backend,
    int width,
    int height,
    const char *title,
    bool vsync_enabled
) {
    unsigned int config_flags = FLAG_WINDOW_RESIZABLE;

    if (backend == NULL) {
        return false;
    }

    memset(backend, 0, sizeof(*backend));
    if (vsync_enabled) {
        config_flags |= FLAG_VSYNC_HINT;
    }
    SetConfigFlags(config_flags);
    InitWindow(width, height, title != NULL ? title : EngineSettings_GetDefaults()->window_title);
    if (!IsWindowReady()) {
        return false;
    }

    backend->window_width = GetScreenWidth();
    backend->window_height = GetScreenHeight();
    backend->target_width = GetRenderWidth();
    backend->target_height = GetRenderHeight();
    backend->instancing_enabled = false;

    backend->render_backend.userdata = backend;
    backend->render_backend.create_texture = raylib_backend_create_texture;
    backend->render_backend.destroy_texture = raylib_backend_destroy_texture;
    backend->render_backend.set_target = raylib_backend_set_target;
    backend->render_backend.reset_target = raylib_backend_reset_target;
    backend->render_backend.clear = raylib_backend_clear;
    backend->render_backend.draw_line = raylib_backend_draw_line;
    backend->render_backend.draw_rect = raylib_backend_draw_rect;
    backend->render_backend.draw_rect_filled = raylib_backend_draw_rect_filled;
    backend->render_backend.draw_triangle_filled = raylib_backend_draw_triangle_filled;
    backend->render_backend.draw_circle = raylib_backend_draw_circle;
    backend->render_backend.draw_oval = raylib_backend_draw_oval;
    backend->render_backend.draw_text = raylib_backend_draw_text;
    backend->render_backend.draw_texture = raylib_backend_draw_texture;
    backend->render_backend.draw_texture_tinted = raylib_backend_draw_texture_tinted;
    backend->render_backend.draw_texture_ex = raylib_backend_draw_texture_ex;
    backend->render_backend.draw_texture_batch = raylib_backend_draw_texture_batch;
    backend->render_backend.draw_tilemap = raylib_backend_draw_tilemap;
    backend->render_backend.push_clip = raylib_backend_push_clip;
    backend->render_backend.pop_clip = raylib_backend_pop_clip;
    backend->render_backend.set_blend_mode = raylib_backend_set_blend_mode;
    backend->render_backend.reset_blend_mode = raylib_backend_reset_blend_mode;
    return true;
}

RaylibBackend* raylib_backend_create(int width, int height, const char* title, bool vsync_enabled) {
    RaylibBackend* backend = (RaylibBackend*)calloc(1U, sizeof(*backend));

    if (backend == NULL) {
        return NULL;
    }
    if (!raylib_backend_init(backend, width, height, title, vsync_enabled)) {
        free(backend);
        return NULL;
    }
    return backend;
}

void raylib_backend_dispose(RaylibBackend *backend) {
    if (backend == NULL) {
        return;
    }
    raylib_backend_release_instancing_state(backend);
    CloseWindow();
    memset(backend, 0, sizeof(*backend));
}

void raylib_backend_destroy(RaylibBackend* backend) {
    if (backend == NULL) {
        return;
    }
    raylib_backend_dispose(backend);
    free(backend);
}

void raylib_backend_pump_events(RaylibBackend *backend) {
    if (backend == NULL) {
        return;
    }

    backend->window_width = GetScreenWidth();
    backend->window_height = GetScreenHeight();
    if (!backend->target_active) {
        backend->target_width = GetRenderWidth();
        backend->target_height = GetRenderHeight();
    }
    backend->should_close = WindowShouldClose();

    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_LEFT] = IsKeyDown(KEY_A);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_RIGHT] = IsKeyDown(KEY_D);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_UP] = IsKeyDown(KEY_W);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_DOWN] = IsKeyDown(KEY_S);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_JUMP] = IsKeyDown(KEY_SPACE);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_ACTION] = IsKeyDown(KEY_E);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_CYCLE_TARGET] = IsKeyDown(KEY_TAB);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_DEBUG_TOGGLE] = IsKeyDown(KEY_ONE);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_DEBUG_WORLD_TOGGLE] = IsKeyDown(KEY_TWO);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_PAN_LEFT] = IsKeyDown(KEY_LEFT);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_PAN_RIGHT] = IsKeyDown(KEY_RIGHT);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_PAN_UP] = IsKeyDown(KEY_UP);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_PAN_DOWN] = IsKeyDown(KEY_DOWN);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_ZOOM_IN] = IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD);
    backend->input_snapshot.buttons[ENGINE_INPUT_ACTION_ZOOM_OUT] = IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT);

    backend->input_snapshot.repeated[ENGINE_INPUT_ACTION_CYCLE_TARGET] = IsKeyPressed(KEY_TAB);
    backend->input_snapshot.repeated[ENGINE_INPUT_ACTION_DEBUG_TOGGLE] = IsKeyPressed(KEY_ONE);
    backend->input_snapshot.repeated[ENGINE_INPUT_ACTION_DEBUG_WORLD_TOGGLE] = IsKeyPressed(KEY_TWO);
    backend->input_snapshot.mouse_x = GetMouseX();
    backend->input_snapshot.mouse_y = GetMouseY();
    backend->input_snapshot.mouse_buttons = 0U;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        backend->input_snapshot.mouse_buttons |= 1U;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        backend->input_snapshot.mouse_buttons |= 2U;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        backend->input_snapshot.mouse_buttons |= 4U;
    }
    backend->input_snapshot.mouse_wheel_delta = GetMouseWheelMove();
}

void raylib_backend_begin_frame(RaylibBackend *backend, Color32 clear_color) {
    if (backend == NULL) {
        return;
    }

    backend->clip_depth = 0;
    backend->frame_active = true;
    BeginDrawing();
    ClearBackground(raylib_unpack_color(clear_color));
}

void raylib_backend_end_frame(RaylibBackend *backend) {
    if (backend == NULL) {
        return;
    }
    EndDrawing();
    backend->frame_active = false;
}

bool raylib_backend_should_close(const RaylibBackend *backend) {
    return backend == NULL || backend->should_close;
}

uint64_t raylib_backend_now_ms(void) {
    return (uint64_t)(GetTime() * 1000.0);
}

EngineInputSnapshot raylib_backend_snapshot_input(const RaylibBackend *backend) {
    EngineInputSnapshot snapshot;

    memset(&snapshot, 0, sizeof(snapshot));
    if (backend != NULL) {
        snapshot = backend->input_snapshot;
    }
    return snapshot;
}

RenderBackend* raylib_backend_get_render_backend(RaylibBackend* backend) {
    return backend != NULL ? &backend->render_backend : NULL;
}

const RenderBackend* raylib_backend_get_render_backend_const(const RaylibBackend* backend) {
    return backend != NULL ? &backend->render_backend : NULL;
}

void raylib_backend_get_window_size(const RaylibBackend* backend, int* out_width, int* out_height) {
    if (out_width != NULL) {
        *out_width = backend != NULL ? backend->window_width : 0;
    }
    if (out_height != NULL) {
        *out_height = backend != NULL ? backend->window_height : 0;
    }
}
