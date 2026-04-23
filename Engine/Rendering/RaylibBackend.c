#include "RaylibBackendInternal.h"

#include "../Core/Profiling.h"
#include "../Settings.h"

#include "../../third_party/raylib/src/external/glad.h"
#include "../Core/Memory.h"

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
static bool raylib_backend_tracy_frame_images_enabled_from_env(void);
static void raylib_backend_tracy_release_frame_images(RaylibBackend* backend);
static void raylib_backend_tracy_capture_frame_image(RaylibBackend* backend);

static bool raylib_backend_tracy_frame_images_enabled_from_env(void)
{
    const char* value = getenv("JIMOKOMI_TRACY_FRAME_IMAGES");

    if (value == NULL || value[0] == '\0')
    {
        return false;
    }

    return strcmp(value, "0") != 0 &&
           strcmp(value, "false") != 0 &&
           strcmp(value, "FALSE") != 0 &&
           strcmp(value, "off") != 0 &&
           strcmp(value, "OFF") != 0;
}

static bool raylib_backend_tracy_frame_images_supported(void)
{
    return glad_glGenBuffers != NULL &&
           glad_glDeleteBuffers != NULL &&
           glad_glBindBuffer != NULL &&
           glad_glBufferData != NULL &&
           glad_glReadPixels != NULL &&
           glad_glMapBufferRange != NULL &&
           glad_glUnmapBuffer != NULL &&
           glad_glFenceSync != NULL &&
           glad_glClientWaitSync != NULL &&
           glad_glDeleteSync != NULL;
}

static int raylib_backend_tracy_frame_image_dimension(int dimension)
{
    if (dimension <= 0)
    {
        return 0;
    }

    return dimension - (dimension % 4);
}

static GLsync raylib_backend_tracy_get_frame_image_fence(const RaylibBackend* backend, unsigned int slot)
{
    return backend != NULL ? (GLsync)(uintptr_t)backend->tracy_frame_image_fences[slot] : NULL;
}

static void raylib_backend_tracy_set_frame_image_fence(RaylibBackend* backend, unsigned int slot, GLsync fence)
{
    if (backend == NULL)
    {
        return;
    }

    backend->tracy_frame_image_fences[slot] = (uintptr_t)fence;
}

static bool raylib_backend_tracy_ensure_frame_images(RaylibBackend* backend)
{
    int frame_image_width = 0;
    int frame_image_height = 0;
    size_t frame_image_bytes = 0U;
    unsigned int index = 0U;

    if (backend == NULL ||
        !backend->tracy_frame_images_enabled ||
        !raylib_backend_tracy_frame_images_supported())
    {
        return false;
    }

    frame_image_width = raylib_backend_tracy_frame_image_dimension(backend->target_width);
    frame_image_height = raylib_backend_tracy_frame_image_dimension(backend->target_height);
    if (frame_image_width <= 0 || frame_image_height <= 0)
    {
        backend->tracy_frame_images_ready = false;
        return false;
    }

    if (backend->tracy_frame_images_ready &&
        backend->tracy_frame_image_width == frame_image_width &&
        backend->tracy_frame_image_height == frame_image_height)
    {
        return true;
    }
    if (backend->tracy_frame_images_ready)
    {
        raylib_backend_tracy_release_frame_images(backend);
    }

    backend->tracy_frame_image_width = frame_image_width;
    backend->tracy_frame_image_height = frame_image_height;
    frame_image_bytes =
        (size_t)backend->tracy_frame_image_width * (size_t)backend->tracy_frame_image_height * 4U;

    glGenBuffers(3, backend->tracy_frame_image_pbos);
    for (index = 0U; index < 3U; ++index)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, backend->tracy_frame_image_pbos[index]);
        glBufferData(GL_PIXEL_PACK_BUFFER, (GLsizeiptr)frame_image_bytes, NULL, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0U);
    backend->tracy_frame_image_write_index = 0U;
    backend->tracy_frame_image_pending_count = 0U;
    backend->tracy_frame_images_ready = true;
    return true;
}

static void raylib_backend_tracy_try_submit_frame_images(RaylibBackend* backend)
{
    while (backend != NULL && backend->tracy_frame_images_ready && backend->tracy_frame_image_pending_count > 0U)
    {
        const unsigned int slot = (backend->tracy_frame_image_write_index + 3U - backend->tracy_frame_image_pending_count) % 3U;
        GLsync fence = raylib_backend_tracy_get_frame_image_fence(backend, slot);
        GLenum wait_result;
        void* image_data = NULL;

        if (fence == NULL)
        {
            break;
        }

        wait_result = glClientWaitSync(fence, 0U, 0U);
        if (wait_result == GL_TIMEOUT_EXPIRED || wait_result == GL_WAIT_FAILED)
        {
            break;
        }

        glDeleteSync(fence);
        raylib_backend_tracy_set_frame_image_fence(backend, slot, NULL);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, backend->tracy_frame_image_pbos[slot]);
        image_data = glMapBufferRange(
            GL_PIXEL_PACK_BUFFER,
            0,
            (GLsizeiptr)((size_t)backend->tracy_frame_image_width * (size_t)backend->tracy_frame_image_height * 4U),
            GL_MAP_READ_BIT
        );
        if (image_data != NULL)
        {
            engine_profile_frame_image(
                image_data,
                (uint16_t)backend->tracy_frame_image_width,
                (uint16_t)backend->tracy_frame_image_height,
                (uint8_t)backend->tracy_frame_image_pending_count,
                true
            );
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0U);
        backend->tracy_frame_image_pending_count -= 1U;
    }
}

static void raylib_backend_tracy_release_frame_images(RaylibBackend* backend)
{
    unsigned int index = 0U;

    if (backend == NULL)
    {
        return;
    }

    for (index = 0U; index < 3U; ++index)
    {
        GLsync fence = raylib_backend_tracy_get_frame_image_fence(backend, index);
        if (fence != NULL)
        {
            glDeleteSync(fence);
            raylib_backend_tracy_set_frame_image_fence(backend, index, NULL);
        }
    }
    if (backend->tracy_frame_images_ready)
    {
        glDeleteBuffers(3, backend->tracy_frame_image_pbos);
    }
    memset(backend->tracy_frame_image_pbos, 0, sizeof(backend->tracy_frame_image_pbos));
    backend->tracy_frame_image_width = 0;
    backend->tracy_frame_image_height = 0;
    backend->tracy_frame_image_write_index = 0U;
    backend->tracy_frame_image_pending_count = 0U;
    backend->tracy_frame_images_ready = false;
}

static void raylib_backend_tracy_capture_frame_image(RaylibBackend* backend)
{
    unsigned int slot = 0U;

    if (backend == NULL || !backend->tracy_frame_images_enabled)
    {
        return;
    }

    if (!engine_profile_is_connected())
    {
        raylib_backend_tracy_release_frame_images(backend);
        return;
    }

    if (!raylib_backend_tracy_ensure_frame_images(backend))
    {
        return;
    }

    raylib_backend_tracy_try_submit_frame_images(backend);
    if (backend->tracy_frame_image_pending_count >= 3U)
    {
        return;
    }

    slot = backend->tracy_frame_image_write_index;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, backend->tracy_frame_image_pbos[slot]);
    glReadPixels(
        0,
        0,
        backend->tracy_frame_image_width,
        backend->tracy_frame_image_height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        NULL
    );
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0U);
    raylib_backend_tracy_set_frame_image_fence(backend, slot, glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0U));
    backend->tracy_frame_image_write_index = (slot + 1U) % 3U;
    backend->tracy_frame_image_pending_count += 1U;
}

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
    EngineProfileGpuZone gpu_zone = { 0 };
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
    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Texture Batch");

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
    engine_profile_gpu_zone_end(&gpu_zone);
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
    EngineProfileGpuZone gpu_zone = { 0 };
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
    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Set Target");
    BeginTextureMode(raylib_texture->target);
    engine_profile_gpu_zone_end(&gpu_zone);
    backend->target_active = true;
    backend->target_width = raylib_texture->base.width;
    backend->target_height = raylib_texture->base.height;
}

static void raylib_backend_reset_target(void *userdata) {
    EngineProfileGpuZone gpu_zone = { 0 };
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
        engine_profile_gpu_zone_begin(&gpu_zone, "GPU Reset Target");
        EndTextureMode();
        engine_profile_gpu_zone_end(&gpu_zone);
        backend->target_active = false;
    }
    backend->target_width = GetRenderWidth();
    backend->target_height = GetRenderHeight();
}

static void raylib_backend_clear(void *userdata, Color32 color) {
    EngineProfileGpuZone gpu_zone = { 0 };
    (void)userdata;
    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Clear");
    ClearBackground(raylib_unpack_color(color));
    engine_profile_gpu_zone_end(&gpu_zone);
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

static void raylib_backend_draw_triangle_textured(
    void *userdata,
    const Texture *texture,
    Vec2 a,
    Vec2 b,
    Vec2 c,
    Vec2 uv_a,
    Vec2 uv_b,
    Vec2 uv_c,
    Color32 tint
) {
    EngineProfileGpuZone gpu_zone = { 0 };
    const RaylibTexture *raylib_texture = raylib_texture_from_const_base(texture);
    Color color = raylib_unpack_color(tint);

    (void)userdata;

    if (raylib_texture == NULL) {
        return;
    }

    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Textured Triangle");
    rlSetTexture(raylib_texture->target.texture.id);
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlTexCoord2f(uv_a.x, uv_a.y);
    rlVertex2f(a.x, a.y);
    rlTexCoord2f(uv_b.x, uv_b.y);
    rlVertex2f(b.x, b.y);
    rlTexCoord2f(uv_c.x, uv_c.y);
    rlVertex2f(c.x, c.y);
    rlEnd();
    rlSetTexture(0U);
    engine_profile_gpu_zone_end(&gpu_zone);
}

static void raylib_backend_draw_triangles(
    void *userdata,
    const Texture *texture,
    const TriangleDrawInstance *triangles,
    size_t triangle_count
) {
    EngineProfileGpuZone gpu_zone = { 0 };
    const RaylibTexture *raylib_texture = texture != NULL
        ? raylib_texture_from_const_base(texture)
        : NULL;
    size_t index;

    (void)userdata;

    if (triangles == NULL || triangle_count == 0U)
    {
        return;
    }
    if (texture != NULL && raylib_texture == NULL)
    {
        return;
    }

    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Triangle Batch");
    rlSetTexture(raylib_texture != NULL ? raylib_texture->target.texture.id : 0U);
    rlBegin(RL_TRIANGLES);
    if (raylib_texture != NULL)
    {
        for (index = 0U; index < triangle_count; ++index)
        {
            const TriangleDrawInstance *triangle = &triangles[index];
            Color color = raylib_unpack_color(triangle->tint);

            rlColor4ub(color.r, color.g, color.b, color.a);
            rlTexCoord2f(triangle->uv_a.x, triangle->uv_a.y);
            rlVertex2f(triangle->a.x, triangle->a.y);
            rlTexCoord2f(triangle->uv_b.x, triangle->uv_b.y);
            rlVertex2f(triangle->b.x, triangle->b.y);
            rlTexCoord2f(triangle->uv_c.x, triangle->uv_c.y);
            rlVertex2f(triangle->c.x, triangle->c.y);
        }
    }
    else
    {
        for (index = 0U; index < triangle_count; ++index)
        {
            const TriangleDrawInstance *triangle = &triangles[index];
            Color color = raylib_unpack_color(triangle->tint);

            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(triangle->a.x, triangle->a.y);
            rlVertex2f(triangle->b.x, triangle->b.y);
            rlVertex2f(triangle->c.x, triangle->c.y);
        }
    }
    rlEnd();
    rlSetTexture(0U);
    engine_profile_gpu_zone_end(&gpu_zone);
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
    EngineProfileGpuZone gpu_zone = { 0 };
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
    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Texture Draw");
    DrawTexturePro(raylib_texture->target.texture, source, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    engine_profile_gpu_zone_end(&gpu_zone);
}

static void raylib_backend_draw_texture_tinted(void *userdata, const Texture *texture, float x, float y, Color32 tint) {
    EngineProfileGpuZone gpu_zone = { 0 };
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
    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Tinted Texture Draw");
    DrawTexturePro(raylib_texture->target.texture, source, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, raylib_unpack_color(tint));
    engine_profile_gpu_zone_end(&gpu_zone);
}

static void raylib_backend_draw_texture_ex(
    void *userdata,
    const Texture *texture,
    Rect dest,
    Vec2 origin,
    float rotation_degrees
) {
    EngineProfileGpuZone gpu_zone = { 0 };
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
    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Texture Draw Ex");
    DrawTexturePro(raylib_texture->target.texture, source, draw_dest, draw_origin, rotation_degrees, WHITE);
    engine_profile_gpu_zone_end(&gpu_zone);
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
    backend->tracy_frame_images_enabled = raylib_backend_tracy_frame_images_enabled_from_env();

    engine_profile_gpu_init();
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
    backend->render_backend.draw_triangle_textured = raylib_backend_draw_triangle_textured;
    backend->render_backend.draw_triangles = raylib_backend_draw_triangles;
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
    raylib_backend_tracy_release_frame_images(backend);
    raylib_backend_release_instancing_state(backend);
    engine_profile_gpu_shutdown();
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
    ENGINE_PROFILE_ZONE_BEGIN(begin_frame_zone, "raylib_backend_begin_frame");
    EngineProfileGpuZone gpu_zone = { 0 };
    if (backend == NULL) {
        ENGINE_PROFILE_ZONE_END(begin_frame_zone);
        return;
    }

    backend->clip_depth = 0;
    backend->frame_active = true;
    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Begin Frame");
    BeginDrawing();
    ClearBackground(raylib_unpack_color(clear_color));
    engine_profile_gpu_zone_end(&gpu_zone);
    ENGINE_PROFILE_ZONE_END(begin_frame_zone);
}

void raylib_backend_end_frame(RaylibBackend *backend) {
    ENGINE_PROFILE_ZONE_BEGIN(end_frame_zone, "raylib_backend_end_frame");
    EngineProfileGpuZone gpu_zone = { 0 };
    if (backend == NULL) {
        ENGINE_PROFILE_ZONE_END(end_frame_zone);
        return;
    }
    raylib_backend_tracy_capture_frame_image(backend);
    engine_profile_gpu_zone_begin(&gpu_zone, "GPU Present");
    EndDrawing();
    engine_profile_gpu_zone_end(&gpu_zone);
    engine_profile_gpu_collect();
    backend->frame_active = false;
    ENGINE_PROFILE_ZONE_END(end_frame_zone);
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

int raylib_backend_get_display_refresh_rate(const RaylibBackend* backend)
{
    int monitor = 0;
    int refresh_rate_hz = 0;

    (void)backend;

    monitor = GetCurrentMonitor();
    refresh_rate_hz = GetMonitorRefreshRate(monitor);
    return refresh_rate_hz > 0 ? refresh_rate_hz : 60;
}
