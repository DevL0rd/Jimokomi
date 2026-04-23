#include "ParticleVisualResources.h"

#include "RendererResources.h"
#include "ResourceDescriptors.h"
#include "Target.h"

#include <math.h>
#include <string.h>

#define PARTICLE_VISUAL_TEXTURE_SIZE 10
#define PARTICLE_VISUAL_TEXTURE_CENTER 5.0f
#define PARTICLE_VISUAL_TEXTURE_RADIUS 4.6f
#define PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY 4096U

typedef struct ParticleSurfaceMeshFieldContext
{
    const PhysicsParticleRenderData* particles;
    size_t particle_count;
    uint32_t fallback_color;
    float min_x;
    float min_y;
    float cell_size;
    float influence_radius;
    size_t vertex_count_x;
    size_t vertex_count;
    size_t worker_count;
    float* values;
    float* red;
    float* green;
    float* blue;
    float* alpha;
} ParticleSurfaceMeshFieldContext;

typedef struct ParticleSurfaceMeshReduceContext
{
    size_t vertex_count;
    size_t worker_count;
    const float* worker_values;
    const float* worker_red;
    const float* worker_green;
    const float* worker_blue;
    const float* worker_alpha;
    float* values;
    float* red;
    float* green;
    float* blue;
    float* alpha;
} ParticleSurfaceMeshReduceContext;

static float particle_surface_mesh_clamp(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static Vec2 particle_surface_mesh_interp_point(
    Vec2 a,
    Vec2 b,
    float value_a,
    float value_b,
    float threshold
)
{
    float span = value_b - value_a;
    float t = fabsf(span) > 0.000001f ? (threshold - value_a) / span : 0.5f;

    t = particle_surface_mesh_clamp(t, 0.0f, 1.0f);
    return (Vec2){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}

static uint8_t particle_surface_mesh_color_channel(uint32_t argb, uint32_t shift)
{
    return (uint8_t)((argb >> shift) & 0xffU);
}

static Color32 particle_surface_mesh_color(
    const ParticleSurfaceMeshBuildInput* input,
    const float* values,
    const float* red,
    const float* green,
    const float* blue,
    const float* alpha,
    size_t a,
    size_t b,
    size_t c,
    size_t d
)
{
    float total = values[a] + values[b] + values[c] + values[d];
    float tint_alpha = (float)particle_surface_mesh_color_channel(input->mesh_tint.value, 24U);
    float r;
    float g;
    float bl;
    float al;

    if (total <= 0.000001f)
    {
        return input->mesh_tint;
    }

    r = (red[a] + red[b] + red[c] + red[d]) / total;
    g = (green[a] + green[b] + green[c] + green[d]) / total;
    bl = (blue[a] + blue[b] + blue[c] + blue[d]) / total;
    al = (alpha[a] + alpha[b] + alpha[c] + alpha[d]) / total;
    if (tint_alpha > 0.0f)
    {
        al = tint_alpha;
    }

    return color_rgba(
        (uint8_t)particle_surface_mesh_clamp(r, 0.0f, 255.0f),
        (uint8_t)particle_surface_mesh_clamp(g, 0.0f, 255.0f),
        (uint8_t)particle_surface_mesh_clamp(bl, 0.0f, 255.0f),
        (uint8_t)particle_surface_mesh_clamp(al, 0.0f, 255.0f)
    );
}

static void particle_surface_mesh_accumulate_range(
    int start_index,
    int end_index,
    uint32_t worker_index,
    void* user_context
)
{
    ParticleSurfaceMeshFieldContext* context = (ParticleSurfaceMeshFieldContext*)user_context;
    size_t worker_offset;
    int particle_index;

    if (context == NULL || context->particles == NULL || worker_index >= context->worker_count)
    {
        return;
    }

    worker_offset = (size_t)worker_index * context->vertex_count;
    if (start_index < 0)
    {
        start_index = 0;
    }
    if (end_index > (int)context->particle_count)
    {
        end_index = (int)context->particle_count;
    }

    for (particle_index = start_index; particle_index < end_index; ++particle_index)
    {
        const PhysicsParticleRenderData* particle = &context->particles[particle_index];
        uint32_t color = particle->color_argb != 0U ? particle->color_argb : context->fallback_color;
        float red = (float)particle_surface_mesh_color_channel(color, 16U);
        float green = (float)particle_surface_mesh_color_channel(color, 8U);
        float blue = (float)particle_surface_mesh_color_channel(color, 0U);
        float alpha = (float)particle_surface_mesh_color_channel(color, 24U);
        int min_vertex_x = (int)floorf((particle->position.x - context->influence_radius - context->min_x) / context->cell_size);
        int max_vertex_x = (int)ceilf((particle->position.x + context->influence_radius - context->min_x) / context->cell_size);
        int min_vertex_y = (int)floorf((particle->position.y - context->influence_radius - context->min_y) / context->cell_size);
        int max_vertex_y = (int)ceilf((particle->position.y + context->influence_radius - context->min_y) / context->cell_size);
        int vertex_y;

        if (min_vertex_x < 0)
        {
            min_vertex_x = 0;
        }
        if (min_vertex_y < 0)
        {
            min_vertex_y = 0;
        }
        if (max_vertex_x > (int)(context->vertex_count_x - 1U))
        {
            max_vertex_x = (int)(context->vertex_count_x - 1U);
        }
        if (max_vertex_y > (int)((context->vertex_count / context->vertex_count_x) - 1U))
        {
            max_vertex_y = (int)((context->vertex_count / context->vertex_count_x) - 1U);
        }

        for (vertex_y = min_vertex_y; vertex_y <= max_vertex_y; ++vertex_y)
        {
            int vertex_x;
            for (vertex_x = min_vertex_x; vertex_x <= max_vertex_x; ++vertex_x)
            {
                float sample_x = context->min_x + (float)vertex_x * context->cell_size;
                float sample_y = context->min_y + (float)vertex_y * context->cell_size;
                float dx = sample_x - particle->position.x;
                float dy = sample_y - particle->position.y;
                float distance_squared = dx * dx + dy * dy;
                float influence_squared = context->influence_radius * context->influence_radius;
                float influence;
                size_t vertex_index;

                if (distance_squared >= influence_squared)
                {
                    continue;
                }

                influence = 1.0f - (distance_squared / influence_squared);
                vertex_index = worker_offset + (size_t)vertex_y * context->vertex_count_x + (size_t)vertex_x;
                context->values[vertex_index] += influence;
                context->red[vertex_index] += red * influence;
                context->green[vertex_index] += green * influence;
                context->blue[vertex_index] += blue * influence;
                context->alpha[vertex_index] += alpha * influence;
            }
        }
    }
}

static void particle_surface_mesh_reduce_range(
    int start_index,
    int end_index,
    uint32_t worker_index,
    void* user_context
)
{
    ParticleSurfaceMeshReduceContext* context = (ParticleSurfaceMeshReduceContext*)user_context;
    int vertex_index;

    (void)worker_index;

    if (context == NULL)
    {
        return;
    }
    if (start_index < 0)
    {
        start_index = 0;
    }
    if (end_index > (int)context->vertex_count)
    {
        end_index = (int)context->vertex_count;
    }

    for (vertex_index = start_index; vertex_index < end_index; ++vertex_index)
    {
        float value = 0.0f;
        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        float alpha = 0.0f;
        size_t worker_index2;

        for (worker_index2 = 0U; worker_index2 < context->worker_count; ++worker_index2)
        {
            size_t source_index = worker_index2 * context->vertex_count + (size_t)vertex_index;
            value += context->worker_values[source_index];
            red += context->worker_red[source_index];
            green += context->worker_green[source_index];
            blue += context->worker_blue[source_index];
            alpha += context->worker_alpha[source_index];
        }

        context->values[vertex_index] = value;
        context->red[vertex_index] = red;
        context->green[vertex_index] = green;
        context->blue[vertex_index] = blue;
        context->alpha[vertex_index] = alpha;
    }
}

bool particle_surface_mesh_build_geometry(
    ProceduralMeshWriter* writer,
    const ProceduralMeshContext* context,
    const ParticleSurfaceMeshBuildInput* input
)
{
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    size_t particle_index;
    size_t cell_count_x;
    size_t cell_count_y;
    size_t vertex_count_x;
    size_t vertex_count_y;
    size_t vertex_count;
    size_t worker_count;
    size_t worker_field_capacity;
    float* worker_values = NULL;
    float* worker_red = NULL;
    float* worker_green = NULL;
    float* worker_blue = NULL;
    float* worker_alpha = NULL;
    float* values = NULL;
    float* red = NULL;
    float* green = NULL;
    float* blue = NULL;
    float* alpha = NULL;
    bool built = false;

    (void)context;

    if (writer == NULL || writer->add_triangle == NULL || input == NULL || input->particles == NULL || input->particle_count == 0U)
    {
        return false;
    }

    min_x = input->particles[0].position.x - input->influence_radius;
    min_y = input->particles[0].position.y - input->influence_radius;
    max_x = input->particles[0].position.x + input->influence_radius;
    max_y = input->particles[0].position.y + input->influence_radius;
    for (particle_index = 1U; particle_index < input->particle_count; ++particle_index)
    {
        const PhysicsParticleRenderData* particle = &input->particles[particle_index];
        min_x = fminf(min_x, particle->position.x - input->influence_radius);
        min_y = fminf(min_y, particle->position.y - input->influence_radius);
        max_x = fmaxf(max_x, particle->position.x + input->influence_radius);
        max_y = fmaxf(max_y, particle->position.y + input->influence_radius);
    }

    cell_count_x = (size_t)ceilf((max_x - min_x) / input->cell_size);
    cell_count_y = (size_t)ceilf((max_y - min_y) / input->cell_size);
    if (cell_count_x == 0U || cell_count_y == 0U)
    {
        return false;
    }

    vertex_count_x = cell_count_x + 1U;
    vertex_count_y = cell_count_y + 1U;
    vertex_count = vertex_count_x * vertex_count_y;
    worker_count = input->task_system != NULL ? (size_t)task_system_get_worker_count(input->task_system) : 1U;
    if (worker_count < 1U)
    {
        worker_count = 1U;
    }
    worker_field_capacity = vertex_count * worker_count;

    values = (float*)calloc(vertex_count > PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY ? vertex_count : PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY, sizeof(*values));
    red = (float*)calloc(vertex_count > PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY ? vertex_count : PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY, sizeof(*red));
    green = (float*)calloc(vertex_count > PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY ? vertex_count : PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY, sizeof(*green));
    blue = (float*)calloc(vertex_count > PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY ? vertex_count : PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY, sizeof(*blue));
    alpha = (float*)calloc(vertex_count > PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY ? vertex_count : PARTICLE_SURFACE_DEFAULT_VERTEX_CAPACITY, sizeof(*alpha));
    worker_values = (float*)calloc(worker_field_capacity, sizeof(*worker_values));
    worker_red = (float*)calloc(worker_field_capacity, sizeof(*worker_red));
    worker_green = (float*)calloc(worker_field_capacity, sizeof(*worker_green));
    worker_blue = (float*)calloc(worker_field_capacity, sizeof(*worker_blue));
    worker_alpha = (float*)calloc(worker_field_capacity, sizeof(*worker_alpha));
    if (values == NULL || red == NULL || green == NULL || blue == NULL || alpha == NULL ||
        worker_values == NULL || worker_red == NULL || worker_green == NULL || worker_blue == NULL || worker_alpha == NULL)
    {
        goto cleanup;
    }

    {
        ParticleSurfaceMeshFieldContext field_context = {
            .particles = input->particles,
            .particle_count = input->particle_count,
            .fallback_color = input->fallback_tint.value,
            .min_x = min_x,
            .min_y = min_y,
            .cell_size = input->cell_size,
            .influence_radius = input->influence_radius,
            .vertex_count_x = vertex_count_x,
            .vertex_count = vertex_count,
            .worker_count = worker_count,
            .values = worker_values,
            .red = worker_red,
            .green = worker_green,
            .blue = worker_blue,
            .alpha = worker_alpha
        };

        if (!task_system_parallel_for(
                input->task_system,
                (int)input->particle_count,
                1,
                particle_surface_mesh_accumulate_range,
                &field_context))
        {
            particle_surface_mesh_accumulate_range(0, (int)input->particle_count, 0U, &field_context);
        }
    }

    {
        ParticleSurfaceMeshReduceContext reduce_context = {
            .vertex_count = vertex_count,
            .worker_count = worker_count,
            .worker_values = worker_values,
            .worker_red = worker_red,
            .worker_green = worker_green,
            .worker_blue = worker_blue,
            .worker_alpha = worker_alpha,
            .values = values,
            .red = red,
            .green = green,
            .blue = blue,
            .alpha = alpha
        };

        if (!task_system_parallel_for(
                input->task_system,
                (int)vertex_count,
                1,
                particle_surface_mesh_reduce_range,
                &reduce_context))
        {
            particle_surface_mesh_reduce_range(0, (int)vertex_count, 0U, &reduce_context);
        }
    }

    for (size_t cell_y = 0U; cell_y < cell_count_y; ++cell_y)
    {
        size_t cell_x;

        for (cell_x = 0U; cell_x < cell_count_x; ++cell_x)
        {
            size_t v0 = cell_y * vertex_count_x + cell_x;
            size_t v1 = v0 + 1U;
            size_t v3 = v0 + vertex_count_x;
            size_t v2 = v3 + 1U;
            float value0 = values[v0];
            float value1 = values[v1];
            float value2 = values[v2];
            float value3 = values[v3];
            bool inside0 = value0 >= input->threshold;
            bool inside1 = value1 >= input->threshold;
            bool inside2 = value2 >= input->threshold;
            bool inside3 = value3 >= input->threshold;
            Vec2 p0 = { min_x + (float)cell_x * input->cell_size, min_y + (float)cell_y * input->cell_size };
            Vec2 p1 = { p0.x + input->cell_size, p0.y };
            Vec2 p2 = { p0.x + input->cell_size, p0.y + input->cell_size };
            Vec2 p3 = { p0.x, p0.y + input->cell_size };
            Vec2 points[8];
            size_t point_count = 0U;
            size_t point_index;
            Color32 color;

            if (!inside0 && !inside1 && !inside2 && !inside3)
            {
                continue;
            }

            if (inside0) points[point_count++] = p0;
            if (inside0 != inside1) points[point_count++] = particle_surface_mesh_interp_point(p0, p1, value0, value1, input->threshold);
            if (inside1) points[point_count++] = p1;
            if (inside1 != inside2) points[point_count++] = particle_surface_mesh_interp_point(p1, p2, value1, value2, input->threshold);
            if (inside2) points[point_count++] = p2;
            if (inside2 != inside3) points[point_count++] = particle_surface_mesh_interp_point(p2, p3, value2, value3, input->threshold);
            if (inside3) points[point_count++] = p3;
            if (inside3 != inside0) points[point_count++] = particle_surface_mesh_interp_point(p3, p0, value3, value0, input->threshold);
            if (point_count < 3U)
            {
                continue;
            }

            color = particle_surface_mesh_color(input, values, red, green, blue, alpha, v0, v1, v2, v3);
            for (point_index = 1U; point_index + 1U < point_count; ++point_index)
            {
                if (!writer->add_triangle(
                        writer->user_data,
                        points[0],
                        points[point_index],
                        points[point_index + 1U],
                        color,
                        input->mesh_layer))
                {
                    goto cleanup;
                }
            }
        }
    }

    built = true;

cleanup:
    free(values);
    free(red);
    free(green);
    free(blue);
    free(alpha);
    free(worker_values);
    free(worker_red);
    free(worker_green);
    free(worker_blue);
    free(worker_alpha);
    return built;
}

static bool particle_surface_mesh_build(
    ProceduralMeshWriter* writer,
    const ProceduralMeshContext* context,
    void* user_data
)
{
    return particle_surface_mesh_build_geometry(
        writer,
        context,
        (const ParticleSurfaceMeshBuildInput*)user_data
    );
}

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
    mesh_desc.build_mesh = particle_surface_mesh_build;
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
