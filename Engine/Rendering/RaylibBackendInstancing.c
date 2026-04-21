#include "RaylibBackendInternal.h"

#include "../../third_party/raylib/src/external/glad.h"

#include <stdlib.h>
#include <string.h>
#include <raylib.h>
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

static RaylibInstancingState* raylib_backend_get_instancing_state(RaylibBackend* backend)
{
    return backend != NULL ? (RaylibInstancingState*)backend->instancing_state : NULL;
}

static bool raylib_backend_has_core_instancing(void)
{
    return glad_glDrawArraysInstanced != NULL && glad_glVertexAttribDivisor != NULL;
}

static bool raylib_backend_has_arb_instancing(void)
{
    return glad_glDrawArraysInstancedARB != NULL && glad_glVertexAttribDivisorARB != NULL;
}

static bool raylib_backend_can_use_instancing(void)
{
    return raylib_backend_has_core_instancing() || raylib_backend_has_arb_instancing();
}

static void raylib_backend_draw_arrays_instanced(int offset, int count, int instances)
{
    if (raylib_backend_has_core_instancing())
    {
        glad_glDrawArraysInstanced(GL_TRIANGLES, offset, count, instances);
    }
    else if (raylib_backend_has_arb_instancing())
    {
        glad_glDrawArraysInstancedARB(GL_TRIANGLES, offset, count, instances);
    }
}

static void raylib_backend_set_attribute_divisor(unsigned int index, unsigned int divisor)
{
    if (raylib_backend_has_core_instancing())
    {
        glad_glVertexAttribDivisor(index, divisor);
    }
    else if (raylib_backend_has_arb_instancing())
    {
        glad_glVertexAttribDivisorARB(index, divisor);
    }
}

static bool raylib_backend_reserve_instance_rects(
    RaylibInstancingState* state,
    size_t required
)
{
    float* next_values = NULL;
    size_t next_capacity = 0U;

    if (state == NULL)
    {
        return false;
    }

    if (state->instance_capacity >= required)
    {
        return true;
    }

    next_capacity = state->instance_capacity > 0U ? state->instance_capacity : 64U;
    while (next_capacity < required)
    {
        next_capacity *= 2U;
    }

    next_values = (float*)realloc(
        state->instance_rect_data,
        next_capacity * 4U * sizeof(*next_values)
    );
    if (next_values == NULL)
    {
        return false;
    }

    state->instance_rect_data = next_values;
    state->instance_capacity = next_capacity;
    return true;
}

static bool raylib_backend_reserve_instance_vbo(
    RaylibInstancingState* state,
    size_t required
)
{
    size_t next_capacity = 0U;

    if (state == NULL)
    {
        return false;
    }

    if (state->instance_vbo != 0U && state->instance_vbo_capacity >= required)
    {
        return true;
    }

    next_capacity = state->instance_vbo_capacity > 0U ? state->instance_vbo_capacity : 64U;
    while (next_capacity < required)
    {
        next_capacity *= 2U;
    }

    if (state->instance_vbo != 0U)
    {
        rlUnloadVertexBuffer(state->instance_vbo);
        state->instance_vbo = 0U;
        state->instance_vbo_capacity = 0U;
    }

    state->instance_vbo = rlLoadVertexBuffer(
        NULL,
        (int)(next_capacity * 4U * sizeof(float)),
        true
    );
    if (state->instance_vbo == 0U)
    {
        return false;
    }

    state->instance_vbo_capacity = next_capacity;
    return true;
}

void raylib_backend_release_instancing_state(RaylibBackend* backend)
{
    RaylibInstancingState* state = raylib_backend_get_instancing_state(backend);

    if (state == NULL)
    {
        return;
    }

    if (state->quad_mesh.vaoId != 0U || state->quad_mesh.vboId != NULL)
    {
        UnloadMesh(state->quad_mesh);
    }
    if (state->instance_vbo != 0U)
    {
        rlUnloadVertexBuffer(state->instance_vbo);
    }
    if (state->shader.id != 0U)
    {
        UnloadShader(state->shader);
    }
    free(state->instance_rect_data);
    free(state);
    backend->instancing_state = NULL;
}

static bool raylib_backend_init_instancing_state(RaylibBackend* backend)
{
    RaylibInstancingState* state = NULL;
    float* vertices = NULL;
    float* texcoords = NULL;

    if (backend == NULL)
    {
        return false;
    }

    if (!backend->instancing_enabled || !raylib_backend_can_use_instancing())
    {
        return false;
    }

    state = raylib_backend_get_instancing_state(backend);
    if (state != NULL && state->ready)
    {
        return true;
    }

    state = (RaylibInstancingState*)calloc(1U, sizeof(*state));
    if (state == NULL)
    {
        return false;
    }

    state->shader = LoadShaderFromMemory(raylib_instancing_vs, raylib_instancing_fs);
    if (!IsShaderValid(state->shader))
    {
        free(state);
        return false;
    }

    vertices = (float*)calloc(18U, sizeof(*vertices));
    texcoords = (float*)calloc(12U, sizeof(*texcoords));
    if (vertices == NULL || texcoords == NULL)
    {
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

    if (state->quad_mesh.vaoId == 0U)
    {
        if (state->quad_mesh.vaoId != 0U || state->quad_mesh.vboId != NULL)
        {
            UnloadMesh(state->quad_mesh);
        }
        if (state->shader.id != 0U)
        {
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

void raylib_backend_draw_surface_batch(
    void *userdata,
    const Surface *surface,
    const SurfaceDrawInstance *instances,
    size_t instance_count
)
{
    RaylibBackend *backend = (RaylibBackend*)userdata;
    RaylibInstancingState* state = raylib_backend_get_instancing_state(backend);
    unsigned int texture_id = 0U;
    size_t index = 0U;

    if (surface == NULL || instances == NULL || instance_count == 0U)
    {
        return;
    }

    if (backend == NULL || !raylib_backend_init_instancing_state(backend))
    {
        state = NULL;
    }
    else
    {
        state = raylib_backend_get_instancing_state(backend);
    }

    if (state == NULL || !state->ready || !raylib_backend_reserve_instance_rects(state, instance_count))
    {
        raylib_backend_draw_surface_batch_fallback(userdata, surface, instances, instance_count);
        return;
    }

    for (index = 1U; index < instance_count; ++index)
    {
        if (instances[index].tint.value != instances[0].tint.value)
        {
            raylib_backend_draw_surface_batch_fallback(userdata, surface, instances, instance_count);
            return;
        }
    }

    for (index = 0U; index < instance_count; ++index)
    {
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

    if (!raylib_backend_reserve_instance_vbo(state, instance_count))
    {
        raylib_backend_draw_surface_batch_fallback(userdata, surface, instances, instance_count);
        return;
    }

    rlDrawRenderBatchActive();
    rlEnableShader(state->shader.id);
    if (state->shader.locs[SHADER_LOC_COLOR_DIFFUSE] != -1)
    {
        Color tint_color;
        tint_color.r = (unsigned char)((instances[0].tint.value >> 16U) & 0xffU);
        tint_color.g = (unsigned char)((instances[0].tint.value >> 8U) & 0xffU);
        tint_color.b = (unsigned char)(instances[0].tint.value & 0xffU);
        if ((instances[0].tint.value & 0xff000000U) != 0U || instances[0].tint.value == 0U)
        {
            tint_color.a = (unsigned char)((instances[0].tint.value >> 24U) & 0xffU);
        }
        else
        {
            tint_color.a = 255U;
        }
        float tint_values[4] = {
            (float)tint_color.r / 255.0f,
            (float)tint_color.g / 255.0f,
            (float)tint_color.b / 255.0f,
            (float)tint_color.a / 255.0f
        };
        rlSetUniform(state->shader.locs[SHADER_LOC_COLOR_DIFFUSE], tint_values, SHADER_UNIFORM_VEC4, 1);
    }
    if (state->shader.locs[SHADER_LOC_MAP_DIFFUSE] != -1)
    {
        int slot = 0;
        rlSetUniform(state->shader.locs[SHADER_LOC_MAP_DIFFUSE], &slot, SHADER_UNIFORM_INT, 1);
    }
    texture_id = raylib_backend_surface_get_texture_id(surface);
    if (texture_id == 0U)
    {
        raylib_backend_draw_surface_batch_fallback(userdata, surface, instances, instance_count);
        return;
    }
    rlActiveTextureSlot(0);
    rlEnableTexture(texture_id);

    rlEnableVertexArray(state->quad_mesh.vaoId);

    if (state->shader.locs[SHADER_LOC_VERTEX_POSITION] != -1)
    {
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

    if (state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01] != -1)
    {
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

    if (state->shader.locs[SHADER_LOC_VERTEX_COLOR] != -1)
    {
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

    if (state->shader.locs[SHADER_LOC_VERTEX_POSITION] != -1)
    {
        rlDisableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_POSITION]);
    }
    if (state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01] != -1)
    {
        rlDisableVertexAttribute((unsigned int)state->shader.locs[SHADER_LOC_VERTEX_TEXCOORD01]);
    }

    rlDisableVertexBuffer();
    rlDisableVertexArray();
    rlDisableTexture();
    rlDisableShader();
    rlDrawRenderBatchActive();
}
