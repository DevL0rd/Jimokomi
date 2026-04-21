#include "RaylibBackend.h"

#include "../../third_party/raylib/src/external/glad.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

typedef struct RaylibInstancingState {
    Mesh quad_mesh;
    Shader shader;
    float* instance_rect_data;
    size_t instance_capacity;
    unsigned int instance_vbo;
    size_t instance_vbo_capacity;
    bool ready;
} RaylibInstancingState;

typedef struct RaylibSurface {
    Surface base;
    RenderTexture2D target;
} RaylibSurface;

static const char* raylib_instancing_vs =
    "#version 140\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec4 vertexColor;\n"
    "in vec4 instanceTransform;\n"
    "out vec2 fragTexCoord;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragColor = vertexColor;\n"
    "    vec2 clipPos = instanceTransform.xy + vertexPosition.xy*instanceTransform.zw;\n"
    "    gl_Position = vec4(clipPos, 0.0, 1.0);\n"
    "}\n";

static const char* raylib_instancing_fs =
    "#version 140\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    finalColor = texture(texture0, fragTexCoord)*fragColor*colDiffuse;\n"
    "}\n";

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

static RaylibBackend *raylib_backend_from_user_data(void *userdata) {
    return (RaylibBackend *)userdata;
}

static RaylibSurface *raylib_surface_from_base(Surface *surface) {
    return (RaylibSurface *)surface;
}

static const RaylibSurface *raylib_surface_from_const_base(const Surface *surface) {
    return (const RaylibSurface *)surface;
}

static RaylibInstancingState* raylib_backend_get_instancing_state(RaylibBackend* backend) {
    return backend != NULL ? (RaylibInstancingState*)backend->instancing_state : NULL;
}

static bool raylib_backend_has_core_instancing(void) {
    return glad_glDrawArraysInstanced != NULL && glad_glVertexAttribDivisor != NULL;
}

static bool raylib_backend_has_arb_instancing(void) {
    return glad_glDrawArraysInstancedARB != NULL && glad_glVertexAttribDivisorARB != NULL;
}

static bool raylib_backend_can_use_instancing(void) {
    return raylib_backend_has_core_instancing() || raylib_backend_has_arb_instancing();
}

static void raylib_backend_draw_arrays_instanced(int offset, int count, int instances) {
    if (raylib_backend_has_core_instancing()) {
        glad_glDrawArraysInstanced(GL_TRIANGLES, offset, count, instances);
    } else if (raylib_backend_has_arb_instancing()) {
        glad_glDrawArraysInstancedARB(GL_TRIANGLES, offset, count, instances);
    }
}

static void raylib_backend_set_attribute_divisor(unsigned int index, unsigned int divisor) {
    if (raylib_backend_has_core_instancing()) {
        glad_glVertexAttribDivisor(index, divisor);
    } else if (raylib_backend_has_arb_instancing()) {
        glad_glVertexAttribDivisorARB(index, divisor);
    }
}

static void raylib_backend_draw_surface_batch_fallback(
    const RaylibSurface* raylib_surface,
    const SurfaceDrawInstance* instances,
    size_t instance_count
) {
    Rectangle source;
    size_t index;

    if (raylib_surface == NULL || instances == NULL) {
        return;
    }

    source.x = 0.0f;
    source.y = 0.0f;
    source.width = (float)raylib_surface->base.width;
    source.height = (float)-raylib_surface->base.height;

    for (index = 0U; index < instance_count; ++index) {
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
            raylib_surface->target.texture,
            source,
            draw_dest,
            draw_origin,
            instances[index].rotation_degrees,
            raylib_unpack_color(instances[index].tint)
        );
    }
}

static bool raylib_backend_reserve_instance_rects(
    RaylibInstancingState* state,
    size_t required
) {
    float* next_values;
    size_t next_capacity;

    if (state == NULL) {
        return false;
    }

    if (state->instance_capacity >= required) {
        return true;
    }

    next_capacity = state->instance_capacity > 0U ? state->instance_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (float*)realloc(
        state->instance_rect_data,
        next_capacity * 4U * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    state->instance_rect_data = next_values;
    state->instance_capacity = next_capacity;
    return true;
}

static bool raylib_backend_reserve_instance_vbo(
    RaylibInstancingState* state,
    size_t required
) {
    size_t next_capacity;

    if (state == NULL) {
        return false;
    }

    if (state->instance_vbo != 0U && state->instance_vbo_capacity >= required) {
        return true;
    }

    next_capacity = state->instance_vbo_capacity > 0U ? state->instance_vbo_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    if (state->instance_vbo != 0U) {
        rlUnloadVertexBuffer(state->instance_vbo);
        state->instance_vbo = 0U;
        state->instance_vbo_capacity = 0U;
    }

    state->instance_vbo = rlLoadVertexBuffer(
        NULL,
        (int)(next_capacity * 4U * sizeof(float)),
        true
    );
    if (state->instance_vbo == 0U) {
        return false;
    }

    state->instance_vbo_capacity = next_capacity;
    return true;
}

static void raylib_backend_release_instancing_state(RaylibBackend* backend) {
    RaylibInstancingState* state = raylib_backend_get_instancing_state(backend);

    if (state == NULL) {
        return;
    }

    if (state->quad_mesh.vaoId != 0U || state->quad_mesh.vboId != NULL) {
        UnloadMesh(state->quad_mesh);
    }
    if (state->instance_vbo != 0U) {
        rlUnloadVertexBuffer(state->instance_vbo);
    }
    if (state->shader.id != 0U) {
        UnloadShader(state->shader);
    }
    free(state->instance_rect_data);
    free(state);
    backend->instancing_state = NULL;
}

static bool raylib_backend_init_instancing_state(RaylibBackend* backend) {
    RaylibInstancingState* state;
    float* vertices;
    float* texcoords;

    if (backend == NULL) {
        return false;
    }

    if (!backend->instancing_enabled || !raylib_backend_can_use_instancing()) {
        return false;
    }

    state = raylib_backend_get_instancing_state(backend);
    if (state != NULL && state->ready) {
        return true;
    }

    state = (RaylibInstancingState*)calloc(1U, sizeof(*state));
    if (state == NULL) {
        return false;
    }

    state->shader = LoadShaderFromMemory(raylib_instancing_vs, raylib_instancing_fs);
    if (!IsShaderValid(state->shader)) {
        free(state);
        return false;
    }

    vertices = (float*)calloc(18U, sizeof(*vertices));
    texcoords = (float*)calloc(12U, sizeof(*texcoords));
    if (vertices == NULL || texcoords == NULL) {
        free(vertices);
        free(texcoords);
        UnloadShader(state->shader);
        free(state);
        return false;
    }

    {
        const float quad_vertices[18] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f
        };
        const float quad_texcoords[12] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        memcpy(vertices, quad_vertices, sizeof(quad_vertices));
        memcpy(texcoords, quad_texcoords, sizeof(quad_texcoords));
    }

    state->quad_mesh.vertexCount = 6;
    state->quad_mesh.triangleCount = 2;
    state->quad_mesh.vertices = vertices;
    state->quad_mesh.texcoords = texcoords;
    UploadMesh(&state->quad_mesh, false);

    if (state->quad_mesh.vaoId == 0U) {
        if (state->quad_mesh.vaoId != 0U || state->quad_mesh.vboId != NULL) {
            UnloadMesh(state->quad_mesh);
        }
        if (state->shader.id != 0U) {
            UnloadShader(state->shader);
        }
        free(state);
        return false;
    }

    state->shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(state->shader, "colDiffuse");
    state->shader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(state->shader, "texture0");
    state->shader.locs[SHADER_LOC_VERTEX_POSITION] = RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION;
    state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01] = RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD;
    state->shader.locs[SHADER_LOC_VERTEX_COLOR] = RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR;
    state->shader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM] = RL_DEFAULT_SHADER_ATTRIB_LOCATION_INSTANCETRANSFORM;
    state->ready = true;
    backend->instancing_state = state;
    return true;
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

static Surface *raylib_backend_create_surface(void *userdata, int width, int height) {
    RaylibBackend *backend = raylib_backend_from_user_data(userdata);
    RaylibSurface *surface;

    if (backend == NULL || width <= 0 || height <= 0) {
        return NULL;
    }

    surface = (RaylibSurface *)calloc(1U, sizeof(*surface));
    if (surface == NULL) {
        return NULL;
    }

    surface->base.width = width;
    surface->base.height = height;
    surface->base.bytes_per_pixel = 4U;
    surface->target = LoadRenderTexture(width, height);
    if (surface->target.id == 0U) {
        free(surface);
        return NULL;
    }

    SetTextureFilter(surface->target.texture, TEXTURE_FILTER_BILINEAR);
    return &surface->base;
}

static void raylib_backend_destroy_surface(void *userdata, Surface *surface) {
    RaylibSurface *raylib_surface = raylib_surface_from_base(surface);
    (void)userdata;

    if (raylib_surface == NULL) {
        return;
    }

    UnloadRenderTexture(raylib_surface->target);
    free(raylib_surface);
}

static void raylib_backend_set_target(void *userdata, Surface *surface) {
    RaylibBackend *backend = raylib_backend_from_user_data(userdata);
    RaylibSurface *raylib_surface = raylib_surface_from_base(surface);

    if (backend == NULL || raylib_surface == NULL) {
        return;
    }

    if (backend->clip_depth > 0) {
        while (backend->clip_depth > 0) {
            EndScissorMode();
            backend->clip_depth -= 1;
        }
    }
    BeginTextureMode(raylib_surface->target);
    backend->target_active = true;
    backend->target_width = raylib_surface->base.width;
    backend->target_height = raylib_surface->base.height;
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

static void raylib_backend_draw_surface(void *userdata, const Surface *surface, float x, float y) {
    const RaylibSurface *raylib_surface = raylib_surface_from_const_base(surface);
    Rectangle source;
    Rectangle dest;
    (void)userdata;

    if (raylib_surface == NULL) {
        return;
    }

    source.x = 0.0f;
    source.y = 0.0f;
    source.width = (float)raylib_surface->base.width;
    source.height = (float)-raylib_surface->base.height;
    dest.x = x;
    dest.y = y;
    dest.width = (float)raylib_surface->base.width;
    dest.height = (float)raylib_surface->base.height;
    DrawTexturePro(raylib_surface->target.texture, source, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
}

static void raylib_backend_draw_surface_tinted(void *userdata, const Surface *surface, float x, float y, Color32 tint) {
    const RaylibSurface *raylib_surface = raylib_surface_from_const_base(surface);
    Rectangle source;
    Rectangle dest;
    (void)userdata;

    if (raylib_surface == NULL) {
        return;
    }

    source.x = 0.0f;
    source.y = 0.0f;
    source.width = (float)raylib_surface->base.width;
    source.height = (float)-raylib_surface->base.height;
    dest.x = x;
    dest.y = y;
    dest.width = (float)raylib_surface->base.width;
    dest.height = (float)raylib_surface->base.height;
    DrawTexturePro(raylib_surface->target.texture, source, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, raylib_unpack_color(tint));
}

static void raylib_backend_draw_surface_ex(
    void *userdata,
    const Surface *surface,
    Rect dest,
    Vec2 origin,
    float rotation_degrees
) {
    const RaylibSurface *raylib_surface = raylib_surface_from_const_base(surface);
    Rectangle source;
    Rectangle draw_dest;
    Vector2 draw_origin;
    (void)userdata;

    if (raylib_surface == NULL) {
        return;
    }

    source.x = 0.0f;
    source.y = 0.0f;
    source.width = (float)raylib_surface->base.width;
    source.height = (float)-raylib_surface->base.height;
    draw_dest.x = dest.x + origin.x;
    draw_dest.y = dest.y + origin.y;
    draw_dest.width = dest.w;
    draw_dest.height = dest.h;
    draw_origin.x = origin.x;
    draw_origin.y = origin.y;
    DrawTexturePro(raylib_surface->target.texture, source, draw_dest, draw_origin, rotation_degrees, WHITE);
}

static void raylib_backend_draw_surface_batch(
    void *userdata,
    const Surface *surface,
    const SurfaceDrawInstance *instances,
    size_t instance_count
) {
    const RaylibSurface *raylib_surface = raylib_surface_from_const_base(surface);
    RaylibBackend *backend = raylib_backend_from_user_data(userdata);
    RaylibInstancingState* state = raylib_backend_get_instancing_state(backend);
    size_t index;

    if (raylib_surface == NULL || instances == NULL || instance_count == 0U) {
        return;
    }

    if (backend == NULL || !raylib_backend_init_instancing_state(backend)) {
        state = NULL;
    } else {
        state = raylib_backend_get_instancing_state(backend);
    }

    if (state == NULL || !state->ready || !raylib_backend_reserve_instance_rects(state, instance_count)) {
        raylib_backend_draw_surface_batch_fallback(raylib_surface, instances, instance_count);
        return;
    }

    for (index = 1U; index < instance_count; ++index) {
        if (instances[index].tint.value != instances[0].tint.value) {
            raylib_backend_draw_surface_batch_fallback(raylib_surface, instances, instance_count);
            return;
        }
    }

    for (index = 0U; index < instance_count; ++index) {
        float center_x = instances[index].dest.x + instances[index].dest.w * 0.5f;
        float center_y = instances[index].dest.y + instances[index].dest.h * 0.5f;
        float clip_size_x = (instances[index].dest.w / (float)backend->target_width) * 2.0f;
        float clip_size_y = (instances[index].dest.h / (float)backend->target_height) * 2.0f;
        float clip_center_x = (center_x / (float)backend->target_width) * 2.0f - 1.0f;
        float clip_center_y = 1.0f - (center_y / (float)backend->target_height) * 2.0f;
        state->instance_rect_data[index * 4U + 0U] = clip_center_x;
        state->instance_rect_data[index * 4U + 1U] = clip_center_y;
        state->instance_rect_data[index * 4U + 2U] = clip_size_x;
        state->instance_rect_data[index * 4U + 3U] = clip_size_y;
    }

    if (!raylib_backend_reserve_instance_vbo(state, instance_count)) {
        raylib_backend_draw_surface_batch_fallback(raylib_surface, instances, instance_count);
        return;
    }

    rlDrawRenderBatchActive();
    rlEnableShader(state->shader.id);
    if (state->shader.locs[SHADER_LOC_COLOR_DIFFUSE] != -1) {
        Color tint_color = raylib_unpack_color(instances[0].tint);
        float tint_values[4] = {
            (float)tint_color.r / 255.0f,
            (float)tint_color.g / 255.0f,
            (float)tint_color.b / 255.0f,
            (float)tint_color.a / 255.0f
        };
        rlSetUniform(state->shader.locs[SHADER_LOC_COLOR_DIFFUSE], tint_values, SHADER_UNIFORM_VEC4, 1);
    }
    if (state->shader.locs[SHADER_LOC_MAP_DIFFUSE] != -1) {
        int slot = 0;
        rlSetUniform(state->shader.locs[SHADER_LOC_MAP_DIFFUSE], &slot, SHADER_UNIFORM_INT, 1);
    }
    rlActiveTextureSlot(0);
    rlEnableTexture(raylib_surface->target.texture.id);

    rlEnableVertexArray(state->quad_mesh.vaoId);

    if (state->shader.locs[SHADER_LOC_VERTEX_POSITION] != -1) {
        rlEnableVertexBuffer(state->quad_mesh.vboId[RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION]);
        rlSetVertexAttribute(
            (unsigned int)state->shader.locs[SHADER_LOC_VERTEX_POSITION],
            3,
            RL_FLOAT,
            0,
            0,
            0
        );
        rlEnableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_POSITION]);
    }

    if (state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01] != -1) {
        rlEnableVertexBuffer(state->quad_mesh.vboId[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD]);
        rlSetVertexAttribute(
            (unsigned int)state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01],
            2,
            RL_FLOAT,
            0,
            0,
            0
        );
        rlEnableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01]);
    }

    if (state->shader.locs[SHADER_LOC_VERTEX_COLOR] != -1) {
        float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        rlSetVertexAttributeDefault(
            (unsigned int)state->shader.locs[SHADER_LOC_VERTEX_COLOR],
            white,
            SHADER_ATTRIB_VEC4,
            4
        );
        rlDisableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_COLOR]);
    }

    rlEnableVertexBuffer(state->instance_vbo);
    rlUpdateVertexBuffer(
        state->instance_vbo,
        state->instance_rect_data,
        (int)(instance_count * 4U * sizeof(*state->instance_rect_data)),
        0
    );

    rlEnableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM]);
    rlSetVertexAttribute(
        (unsigned int)state->shader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM],
        4,
        RL_FLOAT,
        0,
        (int)(4U * sizeof(*state->instance_rect_data)),
        0
    );
    raylib_backend_set_attribute_divisor((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM], 1U);

    raylib_backend_draw_arrays_instanced(0, state->quad_mesh.vertexCount, (int)instance_count);

    raylib_backend_set_attribute_divisor((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM], 0U);
    rlDisableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM]);

    if (state->shader.locs[SHADER_LOC_VERTEX_POSITION] != -1) {
        rlDisableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_POSITION]);
    }
    if (state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01] != -1) {
        rlDisableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01]);
    }

    rlDisableVertexBuffer();
    rlDisableVertexArray();
    rlDisableTexture();
    rlDisableShader();
    rlDrawRenderBatchActive();
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
    const char *title
) {
    if (backend == NULL) {
        return false;
    }

    memset(backend, 0, sizeof(*backend));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title != NULL ? title : "Jimokomi Native Experiment");
    if (!IsWindowReady()) {
        return false;
    }

    backend->window_width = GetScreenWidth();
    backend->window_height = GetScreenHeight();
    backend->target_width = GetRenderWidth();
    backend->target_height = GetRenderHeight();
    backend->instancing_enabled = false;

    backend->render_backend.userdata = backend;
    backend->render_backend.create_surface = raylib_backend_create_surface;
    backend->render_backend.destroy_surface = raylib_backend_destroy_surface;
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
    backend->render_backend.draw_surface = raylib_backend_draw_surface;
    backend->render_backend.draw_surface_tinted = raylib_backend_draw_surface_tinted;
    backend->render_backend.draw_surface_ex = raylib_backend_draw_surface_ex;
    backend->render_backend.draw_surface_batch = raylib_backend_draw_surface_batch;
    backend->render_backend.draw_tilemap = raylib_backend_draw_tilemap;
    backend->render_backend.push_clip = raylib_backend_push_clip;
    backend->render_backend.pop_clip = raylib_backend_pop_clip;
    backend->render_backend.set_blend_mode = raylib_backend_set_blend_mode;
    backend->render_backend.reset_blend_mode = raylib_backend_reset_blend_mode;
    return true;
}

void raylib_backend_dispose(RaylibBackend *backend) {
    if (backend == NULL) {
        return;
    }
    raylib_backend_release_instancing_state(backend);
    CloseWindow();
    memset(backend, 0, sizeof(*backend));
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
