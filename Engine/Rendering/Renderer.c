#include "RendererInternal.h"

#include "DebugOverlay.h"
#include "GeneratedFrame.h"
#include "RendererLifecycleInternal.h"
#include "RendererResources.h"
#include "RendererScratchInternal.h"
#include "RendererSignaturesInternal.h"
#include "RendererStatsInternal.h"
#include "ResourceManagerBake.h"
#include "ResourceManagerRegistry.h"
#include "ResourceManagerStats.h"
#include "../Core/PlatformRuntimeInternal.h"
#include "../Core/Profiling.h"

#include "../Core/Memory.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

double renderer_now_ms(void) {
    return engine_platform_now_ms();
}

static void renderer_draw_material_renderable(Renderer *renderer, RenderBackend *backend, const MaterialRenderable *item, uint64_t now_ms);
static void renderer_draw_procedural_mesh(Renderer* renderer, RenderBackend* backend, const RendererFrame* frame, const ProceduralMeshRenderable* mesh);
static void renderer_draw_triangle(Renderer* renderer, RenderBackend* backend, const TriangleRenderable* triangle, ResourceHandle material_handle, uint64_t now_ms);
static void renderer_draw_line(Renderer* renderer, RenderBackend* backend, const LineRenderable* line);

static bool renderer_reserve_scratch(Renderer* renderer, size_t required_capacity) {
    MaterialRenderable* material_renderables;
    size_t next_capacity;

    if (renderer == NULL) {
        return false;
    }

    if (renderer->scratch->scratch_capacity >= required_capacity) {
        return true;
    }

    next_capacity = renderer->scratch->scratch_capacity > 0U ? renderer->scratch->scratch_capacity : 16U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    material_renderables = (MaterialRenderable*)realloc(renderer->scratch->scratch_material_renderables, next_capacity * sizeof(*material_renderables));
    if (material_renderables == NULL) {
        return false;
    }

    renderer->scratch->scratch_material_renderables = material_renderables;
    renderer->scratch->scratch_capacity = next_capacity;
    return true;
}

static bool renderer_reserve_triangle_draws(Renderer* renderer, size_t required_capacity)
{
    TriangleDrawInstance* triangles = NULL;
    size_t next_capacity = 0U;

    if (renderer == NULL)
    {
        return false;
    }

    if (renderer->scratch->scratch_triangle_capacity >= required_capacity)
    {
        return true;
    }

    next_capacity = renderer->scratch->scratch_triangle_capacity > 0U
        ? renderer->scratch->scratch_triangle_capacity
        : 256U;
    while (next_capacity < required_capacity)
    {
        next_capacity *= 2U;
    }

    triangles = (TriangleDrawInstance*)realloc(
        renderer->scratch->scratch_triangles,
        next_capacity * sizeof(*triangles)
    );
    if (triangles == NULL)
    {
        return false;
    }

    renderer->scratch->scratch_triangles = triangles;
    renderer->scratch->scratch_triangle_capacity = next_capacity;
    return true;
}

static Color32 renderer_multiply_color(Color32 color, Color32 tint)
{
    uint32_t color_alpha = (color.value >> 24U) & 0xffU;
    uint32_t tint_alpha = (tint.value >> 24U) & 0xffU;

    if (color_alpha == 0U && color.value != 0U)
    {
        color_alpha = 255U;
    }
    if (tint_alpha == 0U && tint.value != 0U)
    {
        tint_alpha = 255U;
    }

    return color_rgba(
        (uint8_t)((((color.value >> 16U) & 0xffU) * ((tint.value >> 16U) & 0xffU)) / 255U),
        (uint8_t)((((color.value >> 8U) & 0xffU) * ((tint.value >> 8U) & 0xffU)) / 255U),
        (uint8_t)(((color.value & 0xffU) * (tint.value & 0xffU)) / 255U),
        (uint8_t)((color_alpha * tint_alpha) / 255U)
    );
}

static Vec2 renderer_world_uv(Vec2 point, const Material* material)
{
    float scale_x = material != NULL && fabsf(material->uv_scale_x) > 0.000001f ? material->uv_scale_x : 1.0f;
    float scale_y = material != NULL && fabsf(material->uv_scale_y) > 0.000001f ? material->uv_scale_y : 1.0f;

    return (Vec2){
        point.x / scale_x + (material != NULL ? material->uv_offset_x : 0.0f),
        point.y / scale_y + (material != NULL ? material->uv_offset_y : 0.0f)
    };
}

static const Texture* renderer_get_material_triangle_texture(
    Renderer* renderer,
    const Material* material,
    ResourceHandle material_handle,
    uint64_t now_ms
)
{
    const TextureResource* texture = NULL;
    const ProceduralTextureResource* procedural_texture = NULL;

    if (renderer == NULL || material == NULL)
    {
        return NULL;
    }
    if (material->texture_handle.id == 0U && material->procedural_texture_handle.id == 0U)
    {
        return NULL;
    }

    texture = resource_manager_get_texture(renderer->lifecycle->resource_manager, material->texture_handle);
    if (texture != NULL)
    {
        return texture->texture;
    }
    if (material->procedural_texture_handle.id == 0U)
    {
        return NULL;
    }

    procedural_texture = resource_manager_get_procedural_texture(
        renderer->lifecycle->resource_manager,
        material->procedural_texture_handle
    );
    if (procedural_texture == NULL)
    {
        return NULL;
    }

    return resource_manager_get_or_create_baked_texture(
        renderer->lifecycle->resource_manager,
        material->procedural_texture_handle,
        material_handle,
        material->shader_handle,
        generated_frame_animation_index(&procedural_texture->frames, now_ms),
        BAKED_TEXTURE_PASS_BODY,
        NULL
    );
}

static TriangleDrawInstance renderer_prepare_triangle_draw(
    Renderer* renderer,
    const Material* material,
    const TriangleRenderable* triangle,
    bool use_texture_uv
)
{
    TriangleDrawInstance draw;

    draw.a = triangle->screen_space_valid
        ? triangle->screen_a
        : camera_world_to_screen(&renderer->lifecycle->camera, triangle->a);
    draw.b = triangle->screen_space_valid
        ? triangle->screen_b
        : camera_world_to_screen(&renderer->lifecycle->camera, triangle->b);
    draw.c = triangle->screen_space_valid
        ? triangle->screen_c
        : camera_world_to_screen(&renderer->lifecycle->camera, triangle->c);
    draw.uv_a = triangle->uv_a;
    draw.uv_b = triangle->uv_b;
    draw.uv_c = triangle->uv_c;
    draw.tint = material != NULL && material->base_color != 0U
        ? renderer_multiply_color(triangle->color, (Color32){ material->base_color })
        : triangle->color;

    if (use_texture_uv && material != NULL && material->uv_mode == MATERIAL_UV_WORLD)
    {
        draw.uv_a = renderer_world_uv(triangle->a, material);
        draw.uv_b = renderer_world_uv(triangle->b, material);
        draw.uv_c = renderer_world_uv(triangle->c, material);
    }
    if (((draw.b.x - draw.a.x) * (draw.c.y - draw.a.y) - (draw.b.y - draw.a.y) * (draw.c.x - draw.a.x)) > 0.0f)
    {
        Vec2 swap = draw.b;
        Vec2 uv_swap = draw.uv_b;
        draw.b = draw.c;
        draw.c = swap;
        draw.uv_b = draw.uv_c;
        draw.uv_c = uv_swap;
    }

    return draw;
}

static void renderer_draw_triangle(
    Renderer* renderer,
    RenderBackend* backend,
    const TriangleRenderable* triangle,
    ResourceHandle material_handle,
    uint64_t now_ms
)
{
    Target target;
    const MaterialResource* material_resource = NULL;
    const Material* material = NULL;
    const Texture* draw_texture = NULL;
    Vec2 a;
    Vec2 b;
    Vec2 c;
    Vec2 uv_a;
    Vec2 uv_b;
    Vec2 uv_c;
    Color32 tint;

    if (renderer == NULL || backend == NULL || triangle == NULL || !triangle->visible)
    {
        return;
    }

    material_resource = resource_manager_get_material(renderer->lifecycle->resource_manager, material_handle);
    if (material_resource == NULL)
    {
        return;
    }

    material = &material_resource->value;
    tint = material->base_color != 0U
        ? renderer_multiply_color(triangle->color, (Color32){ material->base_color })
        : triangle->color;

    a = camera_world_to_screen(&renderer->lifecycle->camera, triangle->a);
    b = camera_world_to_screen(&renderer->lifecycle->camera, triangle->b);
    c = camera_world_to_screen(&renderer->lifecycle->camera, triangle->c);
    uv_a = triangle->uv_a;
    uv_b = triangle->uv_b;
    uv_c = triangle->uv_c;
    if (material->uv_mode == MATERIAL_UV_WORLD)
    {
        uv_a = renderer_world_uv(triangle->a, material);
        uv_b = renderer_world_uv(triangle->b, material);
        uv_c = renderer_world_uv(triangle->c, material);
    }
    if (((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) > 0.0f)
    {
        Vec2 swap = b;
        Vec2 uv_swap = uv_b;
        b = c;
        c = swap;
        uv_b = uv_c;
        uv_c = uv_swap;
    }

    draw_texture = renderer_get_material_triangle_texture(renderer, material, material_handle, now_ms);

    if (draw_texture != NULL && backend->draw_triangle_textured != NULL)
    {
        backend->draw_triangle_textured(backend->userdata, draw_texture, a, b, c, uv_a, uv_b, uv_c, tint);
        return;
    }

    target_init(&target, backend, 0.0f, 0.0f);
    target_triangle_filled(&target, a, b, c, tint);
}

static void renderer_draw_line(Renderer* renderer, RenderBackend* backend, const LineRenderable* line)
{
    Target target;
    Vec2 a;
    Vec2 b;

    if (renderer == NULL || backend == NULL || line == NULL || !line->visible)
    {
        return;
    }

    a = camera_world_to_screen(&renderer->lifecycle->camera, line->a);
    b = camera_world_to_screen(&renderer->lifecycle->camera, line->b);
    target_init(&target, backend, 0.0f, 0.0f);
    target_line(&target, a.x, a.y, b.x, b.y, line->color);
}

typedef struct RendererProceduralMeshWriterState {
    Renderer* renderer;
    RenderBackend* backend;
    ResourceHandle material_handle;
    uint64_t now_ms;
} RendererProceduralMeshWriterState;

static bool renderer_procedural_mesh_add_triangle(
    void* user_data,
    Vec2 a,
    Vec2 b,
    Vec2 c,
    Color32 color,
    int layer
) {
    RendererProceduralMeshWriterState* state = (RendererProceduralMeshWriterState*)user_data;
    TriangleRenderable triangle;

    if (state == NULL)
    {
        return false;
    }

    triangle.a = a;
    triangle.b = b;
    triangle.c = c;
    triangle.color = color;
    triangle.layer = layer;
    triangle.visible = true;
    renderer_draw_triangle(state->renderer, state->backend, &triangle, state->material_handle, state->now_ms);
    return true;
}

static bool renderer_procedural_mesh_add_line(
    void* user_data,
    Vec2 a,
    Vec2 b,
    Color32 color,
    int layer
) {
    RendererProceduralMeshWriterState* state = (RendererProceduralMeshWriterState*)user_data;
    LineRenderable line;

    if (state == NULL)
    {
        return false;
    }

    line.a = a;
    line.b = b;
    line.color = color;
    line.layer = layer;
    line.visible = true;
    renderer_draw_line(state->renderer, state->backend, &line);
    return true;
}

static void renderer_draw_procedural_mesh(
    Renderer* renderer,
    RenderBackend* backend,
    const RendererFrame* frame,
    const ProceduralMeshRenderable* mesh
) {
    const MaterialResource* material;
    const ProceduralMeshResource* source = NULL;
    uint32_t frame_index;
    size_t index;

    if (renderer == NULL || backend == NULL || frame == NULL || mesh == NULL || !mesh->visible)
    {
        return;
    }

    material = resource_manager_get_material(renderer->lifecycle->resource_manager, mesh->material_handle);
    if (material == NULL)
    {
        return;
    }

    if (mesh->triangle_count > 0U || mesh->line_count > 0U)
    {
        if (mesh->triangle_count > 0U &&
            backend->draw_triangles != NULL &&
            renderer_reserve_triangle_draws(renderer, mesh->triangle_count))
        {
            const Texture* texture = renderer_get_material_triangle_texture(
                renderer,
                &material->value,
                mesh->material_handle,
                frame->now_ms
            );
            bool use_texture_uv = texture != NULL;
            size_t draw_count = 0U;

            for (index = 0U; index < mesh->triangle_count; ++index)
            {
                size_t triangle_index = mesh->triangle_start + index;
                if (triangle_index < frame->triangle_count)
                {
                    renderer->scratch->scratch_triangles[draw_count++] = renderer_prepare_triangle_draw(
                        renderer,
                        &material->value,
                        &frame->triangles[triangle_index],
                        use_texture_uv
                    );
                }
            }
            backend->draw_triangles(
                backend->userdata,
                texture,
                renderer->scratch->scratch_triangles,
                draw_count
            );
        }
        else
        {
            for (index = 0U; index < mesh->triangle_count; ++index)
            {
                size_t triangle_index = mesh->triangle_start + index;
                if (triangle_index < frame->triangle_count)
                {
                    renderer_draw_triangle(renderer, backend, &frame->triangles[triangle_index], mesh->material_handle, frame->now_ms);
                }
            }
        }
        for (index = 0U; index < mesh->line_count; ++index)
        {
            size_t line_index = mesh->line_start + index;
            if (line_index < frame->line_count)
            {
                renderer_draw_line(renderer, backend, &frame->lines[line_index]);
            }
        }
        return;
    }

    source = resource_manager_get_procedural_mesh(
        renderer->lifecycle->resource_manager,
        mesh->procedural_mesh_handle
    );
    if (source == NULL)
    {
        return;
    }

    if (source->build_mesh == NULL)
    {
        return;
    }

    frame_index = generated_frame_animation_index(&source->frames, frame->now_ms);
    if (source->frames.cache_policy != BAKE_POLICY_DISABLED)
    {
        const Mesh* geometry = resource_manager_get_or_create_baked_mesh(
            renderer->lifecycle->resource_manager,
            mesh->procedural_mesh_handle,
            mesh->instance_signature,
            frame_index,
            mesh->user_data
        );
        if (geometry == NULL)
        {
            return;
        }
        if (geometry->triangle_count > 0U &&
            backend->draw_triangles != NULL &&
            renderer_reserve_triangle_draws(renderer, geometry->triangle_count))
        {
            const Texture* texture = renderer_get_material_triangle_texture(
                renderer,
                &material->value,
                mesh->material_handle,
                frame->now_ms
            );
            bool use_texture_uv = texture != NULL;
            for (index = 0U; index < geometry->triangle_count; ++index)
            {
                renderer->scratch->scratch_triangles[index] = renderer_prepare_triangle_draw(
                    renderer,
                    &material->value,
                    &geometry->triangles[index],
                    use_texture_uv
                );
            }
            backend->draw_triangles(
                backend->userdata,
                texture,
                renderer->scratch->scratch_triangles,
                geometry->triangle_count
            );
        }
        else
        {
            for (index = 0U; index < geometry->triangle_count; ++index)
            {
                renderer_draw_triangle(renderer, backend, &geometry->triangles[index], mesh->material_handle, frame->now_ms);
            }
        }
        for (index = 0U; index < geometry->line_count; ++index)
        {
            renderer_draw_line(renderer, backend, &geometry->lines[index]);
        }
    }
    else
    {
        RendererProceduralMeshWriterState writer_state;
        ProceduralMeshWriter writer;
        ProceduralMeshContext context;

        writer_state.renderer = renderer;
        writer_state.backend = backend;
        writer_state.material_handle = mesh->material_handle;
        writer_state.now_ms = frame->now_ms;
        writer.user_data = &writer_state;
        writer.add_triangle = renderer_procedural_mesh_add_triangle;
        writer.add_line = renderer_procedural_mesh_add_line;

        memset(&context, 0, sizeof(context));
        context.now_ms = frame->now_ms;
        context.time_seconds = (float)frame->now_ms / 1000.0f;
        context.animation_fps = source->frames.animation_fps;
        context.frame_index = frame_index;
        (void)source->build_mesh(&writer, &context, mesh->user_data);
    }
}

static void renderer_draw_material_renderable(Renderer *renderer, RenderBackend *backend, const MaterialRenderable *item, uint64_t now_ms) {
    const ProceduralTextureResource* source;
    const MaterialResource* material;
    const ShaderResource* shader;
    const TextureResource* texture;
    const Texture* baked_body;
    const Texture* baked_overlay;
    Target target;
    ProceduralTextureContext context;
    Material live_material;
    Vec2 screen;
    Rect dest;
    float width;
    float height;
    Vec2 screen_size;
    double started_ms;

    if (renderer == NULL || backend == NULL || item == NULL || !item->visible) {
        return;
    }

    material = resource_manager_get_material(renderer->lifecycle->resource_manager, item->material_handle);
    if (material == NULL) {
        return;
    }

    texture = resource_manager_get_texture(renderer->lifecycle->resource_manager, material->value.texture_handle);
    if (texture != NULL && texture->texture != NULL) {
        TextureDrawInstance instance;

        width = (float)texture->texture->width;
        height = (float)texture->texture->height;
        screen_size = camera_world_size_to_screen(&renderer->lifecycle->camera, (Vec2){ width, height });
        screen = camera_world_to_screen(&renderer->lifecycle->camera, (Vec2){ item->x, item->y });
        dest.x = screen.x - screen_size.x * item->anchor_x;
        dest.y = screen.y - screen_size.y * item->anchor_y;
        dest.w = screen_size.x;
        dest.h = screen_size.y;
        instance.dest = dest;
        instance.origin.x = screen_size.x * item->anchor_x;
        instance.origin.y = screen_size.y * item->anchor_y;
        instance.rotation_degrees = item->angle_radians * (180.0f / 3.14159265359f);
        instance.tint = item->tint.value != 0U ? item->tint : (Color32){ 0xffffffffU };
        if (backend->draw_texture_batch != NULL) {
            started_ms = renderer_now_ms();
            backend->draw_texture_batch(backend->userdata, texture->texture, &instance, 1U);
            renderer->stats->last_body_draw_ms += renderer_now_ms() - started_ms;
        }
        return;
    }

    source = resource_manager_get_procedural_texture(renderer->lifecycle->resource_manager, material->value.procedural_texture_handle);
    shader = resource_manager_get_shader(renderer->lifecycle->resource_manager, material->value.shader_handle);
    if (source == NULL || material == NULL || source->draw_body == NULL) {
        return;
    }

    width = (float)source->width;
    height = (float)source->height;
    screen_size = camera_world_size_to_screen(&renderer->lifecycle->camera, (Vec2){ width, height });
    screen = camera_world_to_screen(&renderer->lifecycle->camera, (Vec2){ item->x, item->y });
    dest.x = screen.x - screen_size.x * item->anchor_x;
    dest.y = screen.y - screen_size.y * item->anchor_y;
    dest.w = screen_size.x;
    dest.h = screen_size.y;
    memset(&context, 0, sizeof(context));
    context.now_ms = now_ms;
    context.time_seconds = (float)now_ms / 1000.0f;
    context.animation_fps = source->frames.animation_fps;
    context.frame_index = generated_frame_animation_index(&source->frames, now_ms);
    context.angle_radians = item->angle_radians;
    live_material = material->value;
    if (source->bake_ignores_material)
    {
        uint32_t tint = item->tint.value != 0U ? item->tint.value : 0xffffffffU;
        live_material.base_color = tint;
        live_material.accent_color = tint;
        live_material.glow_color = tint;
        live_material.glare_color = tint;
    }
    context.material = &live_material;
    context.shader_style = shader != NULL ? shader->style : material->value.shader_style;
    if (source->frames.cache_policy != BAKE_POLICY_DISABLED) {
        baked_body = resource_manager_get_or_create_baked_texture(
            renderer->lifecycle->resource_manager,
            material->value.procedural_texture_handle,
            item->material_handle,
            material->value.shader_handle,
            context.frame_index,
            BAKED_TEXTURE_PASS_BODY,
            item->user_data
        );
        baked_overlay = NULL;
        if (source->draw_overlay != NULL) {
            baked_overlay = resource_manager_get_or_create_baked_texture(
                renderer->lifecycle->resource_manager,
                material->value.procedural_texture_handle,
                item->material_handle,
                material->value.shader_handle,
                context.frame_index,
                BAKED_TEXTURE_PASS_OVERLAY,
                item->user_data
            );
        }
        if (baked_body != NULL && (source->draw_overlay == NULL || baked_overlay != NULL)) {
            started_ms = renderer_now_ms();
            target_init(&target, backend, 0.0f, 0.0f);
            target_texture_ex(
                &target,
                baked_body,
                dest,
                (Vec2){ screen_size.x * item->anchor_x, screen_size.y * item->anchor_y },
                item->angle_radians * (180.0f / 3.14159265359f)
            );
            renderer->stats->last_body_draw_ms += renderer_now_ms() - started_ms;
            if (source->draw_overlay != NULL) {
                started_ms = renderer_now_ms();
                target_init(&target, backend, 0.0f, 0.0f);
                target_texture_ex(
                    &target,
                    baked_overlay,
                    dest,
                    (Vec2){ screen_size.x * item->anchor_x, screen_size.y * item->anchor_y },
                    item->angle_radians * (180.0f / 3.14159265359f)
                );
                renderer->stats->last_overlay_draw_ms += renderer_now_ms() - started_ms;
                renderer->stats->last_overlay_draw_count += 1U;
            }
        }
    } else {
        target_init_scaled(
            &target,
            backend,
            dest.x,
            dest.y,
            width > 0.0f ? screen_size.x / width : 1.0f,
            height > 0.0f ? screen_size.y / height : 1.0f
        );
        started_ms = renderer_now_ms();
        source->draw_body(&target, &context, item->user_data);
        renderer->stats->last_body_draw_ms += renderer_now_ms() - started_ms;
        if (source->draw_overlay != NULL) {
            target_init_scaled(
                &target,
                backend,
                dest.x,
                dest.y,
                width > 0.0f ? screen_size.x / width : 1.0f,
                height > 0.0f ? screen_size.y / height : 1.0f
            );
            started_ms = renderer_now_ms();
            source->draw_overlay(&target, &context, item->user_data);
            renderer->stats->last_overlay_draw_ms += renderer_now_ms() - started_ms;
            renderer->stats->last_overlay_draw_count += 1U;
        }
    }
    renderer->stats->last_material_renderable_item_count += 1U;
}

static int renderer_compare_material_layer(const void *left, const void *right) {
    const MaterialRenderable *a = (const MaterialRenderable *)left;
    const MaterialRenderable *b = (const MaterialRenderable *)right;
    return (a->layer > b->layer) - (a->layer < b->layer);
}

static double renderer_compute_bake_budget_ms(double frame_started_ms, float target_fps) {
    double target_frame_ms;
    double elapsed_ms;

    if (target_fps <= 0.0f) {
        target_fps = 60.0f;
    }

    target_frame_ms = 1000.0 / (double)target_fps;
    elapsed_ms = renderer_now_ms() - frame_started_ms;
    return elapsed_ms < target_frame_ms ? target_frame_ms - elapsed_ms : 0.0;
}

void renderer_init(Renderer *renderer, RenderBackend *backend, const RendererConfig *config) {
    (void)backend;
    if (renderer == NULL) {
        return;
    }
    memset(renderer, 0, sizeof(*renderer));
    renderer->lifecycle = (RendererLifecycleState*)calloc(1U, sizeof(*renderer->lifecycle));
    renderer->scratch = (RendererScratchState*)calloc(1U, sizeof(*renderer->scratch));
    renderer->stats = (RendererStatsState*)calloc(1U, sizeof(*renderer->stats));
    renderer->signatures = (RendererSignatureState*)calloc(1U, sizeof(*renderer->signatures));
    if (renderer->lifecycle == NULL || renderer->scratch == NULL || renderer->stats == NULL || renderer->signatures == NULL) {
        free(renderer->lifecycle);
        free(renderer->scratch);
        free(renderer->stats);
        free(renderer->signatures);
        memset(renderer, 0, sizeof(*renderer));
        return;
    }
    renderer->lifecycle->resource_manager = resource_manager_create(backend);
    renderer->lifecycle->debug_overlay = debug_overlay_create();
    if (renderer->lifecycle->resource_manager == NULL || renderer->lifecycle->debug_overlay == NULL) {
        resource_manager_destroy(renderer->lifecycle->resource_manager);
        debug_overlay_destroy(renderer->lifecycle->debug_overlay);
        memset(renderer, 0, sizeof(*renderer));
        return;
    }
    renderer->lifecycle->view_width = config != NULL && config->view_width > 0 ? config->view_width : 480;
    renderer->lifecycle->view_height = config != NULL && config->view_height > 0 ? config->view_height : 270;
    renderer->lifecycle->prebake_target_fps = config != NULL && config->prebake_target_fps > 0.0f
        ? config->prebake_target_fps
        : 60.0f;
    resource_manager_set_bake_time_budget(renderer->lifecycle->resource_manager, 1000.0 / (double)renderer->lifecycle->prebake_target_fps);
    if (config != NULL) {
        resource_manager_set_bake_admission_thresholds(
            renderer->lifecycle->resource_manager,
            config->prebake_admission_total_hits,
            config->prebake_admission_frame_hits
        );
    }
    camera_init(&renderer->lifecycle->camera, 0.0f, 0.0f, (float)renderer->lifecycle->view_width, (float)renderer->lifecycle->view_height, 0.12f);
}

Renderer* renderer_create(RenderBackend *backend, const RendererConfig *config) {
    Renderer* renderer = (Renderer*)calloc(1, sizeof(*renderer));
    if (renderer == NULL) {
        return NULL;
    }

    renderer_init(renderer, backend, config);
    if (renderer->lifecycle == NULL ||
        renderer->scratch == NULL ||
        renderer->stats == NULL ||
        renderer->signatures == NULL ||
        renderer->lifecycle->resource_manager == NULL ||
        renderer->lifecycle->debug_overlay == NULL) {
        renderer_dispose(renderer);
        free(renderer);
        return NULL;
    }

    return renderer;
}

void renderer_dispose(Renderer *renderer) {
    if (renderer == NULL) {
        return;
    }
    if (renderer->lifecycle != NULL) {
        resource_manager_destroy(renderer->lifecycle->resource_manager);
        debug_overlay_destroy(renderer->lifecycle->debug_overlay);
    }
    if (renderer->scratch != NULL) {
        free(renderer->scratch->scratch_material_renderables);
        free(renderer->scratch->scratch_instances);
        free(renderer->scratch->scratch_triangles);
    }
    free(renderer->lifecycle);
    free(renderer->scratch);
    free(renderer->stats);
    free(renderer->signatures);
    memset(renderer, 0, sizeof(*renderer));
}

void renderer_destroy(Renderer* renderer) {
    if (renderer == NULL) {
        return;
    }

    renderer_dispose(renderer);
    free(renderer);
}

Camera* renderer_get_camera(Renderer* renderer) {
    return renderer != NULL ? &renderer->lifecycle->camera : NULL;
}

const Camera* renderer_get_camera_const(const Renderer* renderer) {
    return renderer != NULL ? &renderer->lifecycle->camera : NULL;
}

void renderer_get_viewport_size(const Renderer* renderer, int* out_width, int* out_height) {
    if (out_width != NULL) {
        *out_width = renderer != NULL ? renderer->lifecycle->view_width : 0;
    }
    if (out_height != NULL) {
        *out_height = renderer != NULL ? renderer->lifecycle->view_height : 0;
    }
}

void renderer_set_viewport_size(Renderer* renderer, int width, int height, float min_view_width, float min_view_height) {
    Camera* camera;
    float center_x;
    float center_y;
    float aspect;
    float next_view_width;
    float next_view_height;

    if (renderer == NULL || renderer->lifecycle == NULL || width <= 0 || height <= 0) {
        return;
    }

    camera = &renderer->lifecycle->camera;
    renderer->lifecycle->view_width = width;
    renderer->lifecycle->view_height = height;
    camera->viewport_width = (float)width;
    camera->viewport_height = (float)height;

    if (camera->view_width <= 0.0f || camera->view_height <= 0.0f) {
        return;
    }

    aspect = (float)width / (float)height;
    next_view_height = camera->view_height;
    next_view_width = next_view_height * aspect;
    if (next_view_width < min_view_width) {
        next_view_width = min_view_width;
        next_view_height = next_view_width / aspect;
    }
    if (next_view_height < min_view_height) {
        next_view_height = min_view_height;
        next_view_width = next_view_height * aspect;
    }
    if (camera->world_bounds.w > 0.0f && next_view_width > camera->world_bounds.w) {
        next_view_width = camera->world_bounds.w;
        next_view_height = next_view_width / aspect;
    }
    if (camera->world_bounds.h > 0.0f && next_view_height > camera->world_bounds.h) {
        next_view_height = camera->world_bounds.h;
        next_view_width = next_view_height * aspect;
    }
    if (next_view_width < min_view_width) {
        next_view_width = min_view_width;
        next_view_height = next_view_width / aspect;
    }
    if (next_view_height < min_view_height) {
        next_view_height = min_view_height;
        next_view_width = next_view_height * aspect;
    }

    if (fabsf(next_view_width - camera->view_width) <= 0.0001f &&
        fabsf(next_view_height - camera->view_height) <= 0.0001f) {
        return;
    }

    center_x = camera->x + camera->view_width * 0.5f;
    center_y = camera->y + camera->view_height * 0.5f;
    camera->view_width = next_view_width;
    camera->view_height = next_view_height;
    camera->x = center_x - camera->view_width * 0.5f;
    camera->y = center_y - camera->view_height * 0.5f;
    camera_clamp_to_bounds(camera, min_view_width, min_view_height);
}

void renderer_toggle_debug_overlay(Renderer* renderer) {
    if (renderer != NULL) {
        debug_overlay_toggle(renderer->lifecycle->debug_overlay);
    }
}

void renderer_toggle_debug_world_gizmos(Renderer* renderer) {
    if (renderer != NULL) {
        debug_overlay_toggle_world_gizmos(renderer->lifecycle->debug_overlay);
    }
}

bool renderer_debug_overlay_is_ui_visible(const Renderer* renderer) {
    return renderer != NULL && debug_overlay_is_ui_visible(renderer->lifecycle->debug_overlay);
}

bool renderer_debug_overlay_is_world_gizmos_visible(const Renderer* renderer) {
    return renderer != NULL && debug_overlay_is_world_gizmos_visible(renderer->lifecycle->debug_overlay);
}

void renderer_debug_overlay_handle_input(
    Renderer* renderer,
    const EngineInput* input,
    bool has_selection,
    int window_width,
    int window_height
) {
    if (renderer != NULL) {
        debug_overlay_handle_input(renderer->lifecycle->debug_overlay, input, has_selection, window_width, window_height);
    }
}

bool renderer_debug_overlay_is_pointer_over_ui(
    const Renderer* renderer,
    bool has_selection,
    float mouse_x,
    float mouse_y,
    int window_width,
    int window_height
) {
    return renderer != NULL &&
           debug_overlay_is_pointer_over_ui(renderer->lifecycle->debug_overlay, has_selection, mouse_x, mouse_y, window_width, window_height);
}

void renderer_draw_debug_overlay_ui(
    Renderer* renderer,
    RenderBackend* backend,
    const DebugOverlaySnapshot* snapshot,
    const struct EngineStatsSnapshot* engine_stats,
    const DebugEntityView* selected_entity,
    int window_width,
    int window_height
) {
    if (renderer != NULL) {
        debug_overlay_draw_ui(renderer->lifecycle->debug_overlay, backend, snapshot, engine_stats, selected_entity, window_width, window_height);
    }
}

ResourceHandle renderer_register_texture_from_builder(
    Renderer* renderer,
    const char* name,
    int width,
    int height,
    RendererTextureBuildFn build_texture,
    void* user_data
) {
    return renderer != NULL
        ? resource_manager_register_texture_from_builder(
            renderer->lifecycle->resource_manager,
            name,
            width,
            height,
            (ResourceTextureBuildFn)build_texture,
            user_data
        )
        : (ResourceHandle){ 0U };
}

ResourceHandle renderer_register_material(Renderer* renderer, const char* name, const Material* material) {
    return renderer != NULL
        ? resource_manager_register_material(renderer->lifecycle->resource_manager, name, material)
        : (ResourceHandle){ 0U };
}

ResourceHandle renderer_register_shader(Renderer* renderer, const char* name, ShaderStyle style) {
    return renderer != NULL
        ? resource_manager_register_shader(renderer->lifecycle->resource_manager, name, style)
        : (ResourceHandle){ 0U };
}

ResourceHandle renderer_register_procedural_texture(
    Renderer* renderer,
    const char* name,
    const ProceduralTextureDesc* desc
) {
    return renderer != NULL
        ? resource_manager_register_procedural_texture(renderer->lifecycle->resource_manager, name, desc)
        : (ResourceHandle){ 0U };
}

ResourceHandle renderer_register_procedural_mesh(
    Renderer* renderer,
    const char* name,
    const ProceduralMeshDesc* desc
) {
    return renderer != NULL
        ? resource_manager_register_procedural_mesh(renderer->lifecycle->resource_manager, name, desc)
        : (ResourceHandle){ 0U };
}

void renderer_get_stats_snapshot(const Renderer* renderer, RendererStatsSnapshot* out_snapshot) {
    if (out_snapshot == NULL) {
        return;
    }
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    if (renderer == NULL) {
        return;
    }

    out_snapshot->material_renderable_count = renderer->stats->last_material_renderable_count;
    out_snapshot->material_renderable_draw_count = renderer->stats->last_material_renderable_draw_count;
    out_snapshot->visible_material_renderable_count = renderer->stats->last_visible_material_renderable_count;
    out_snapshot->overlay_draw_count = renderer->stats->last_overlay_draw_count;
    out_snapshot->material_renderable_item_count = renderer->stats->last_material_renderable_item_count;
    out_snapshot->baked_texture_count = renderer->stats->last_baked_texture_count;
    out_snapshot->instanced_batch_count = renderer->stats->last_instanced_batch_count;
    out_snapshot->instanced_draw_count = renderer->stats->last_instanced_draw_count;
    out_snapshot->sort_ms = renderer->stats->last_sort_ms;
    out_snapshot->visibility_ms = renderer->stats->last_visibility_ms;
    out_snapshot->body_draw_ms = renderer->stats->last_body_draw_ms;
    out_snapshot->overlay_draw_ms = renderer->stats->last_overlay_draw_ms;
    out_snapshot->instance_draw_ms = renderer->stats->last_instance_draw_ms;
}

void renderer_draw(Renderer *renderer, RenderBackend *backend, const RendererFrame *frame) {
    ENGINE_PROFILE_ZONE_BEGIN(renderer_draw_zone, "renderer_draw");
    size_t index;
    bool needs_sort = false;
    bool batching_enabled = false;
    const MaterialRenderable* draw_material_renderables = NULL;
    uint64_t frame_signature;
    uint64_t sort_signature;
    uint64_t overlay_signature;
    uint64_t instance_signature;
    bool material_renderables_sorted_by_layer = true;
    double frame_started_ms;
    if (renderer == NULL || backend == NULL || frame == NULL) {
        ENGINE_PROFILE_ZONE_END(renderer_draw_zone);
        return;
    }

    frame_started_ms = renderer_now_ms();

    {
        ENGINE_PROFILE_ZONE_BEGIN(signature_zone, "renderer_draw_signatures");
        renderer_compute_frame_signatures(frame, &frame_signature, &sort_signature, &instance_signature, &material_renderables_sorted_by_layer);
        overlay_signature = renderer_compute_overlay_signature(frame);
        renderer->signatures->dirty_flags = RENDERER_DIRTY_NONE;
        if (frame_signature != renderer->signatures->last_frame_signature) {
            renderer->signatures->dirty_flags |= RENDERER_DIRTY_FRAME_LIST;
        }
        if (sort_signature != renderer->signatures->last_sort_signature) {
            renderer->signatures->dirty_flags |= RENDERER_DIRTY_SORT;
        }
        if (overlay_signature != renderer->signatures->last_overlay_signature) {
            renderer->signatures->dirty_flags |= RENDERER_DIRTY_OVERLAY_LIST;
        }
        if (instance_signature != renderer->signatures->last_instance_batch_signature) {
            renderer->signatures->dirty_flags |= RENDERER_DIRTY_INSTANCE_BATCH;
        }
        if (frame->backdrop_signature != renderer->signatures->last_backdrop_signature) {
            renderer->signatures->dirty_flags |= RENDERER_DIRTY_BACKDROP;
        }
        if (frame->metadata_dirty || frame->metadata_signature != renderer->signatures->last_metadata_signature) {
            renderer->signatures->dirty_flags |= RENDERER_DIRTY_SNAPSHOT_METADATA;
        }
        ENGINE_PROFILE_ZONE_END(signature_zone);
    }

    renderer->stats->last_material_renderable_count = frame->material_renderable_count;
    renderer->stats->last_material_renderable_draw_count = 0U;
    renderer->stats->last_visible_material_renderable_count = 0U;
    renderer->stats->last_overlay_draw_count = 0U;
    renderer->stats->last_material_renderable_item_count = 0U;
    renderer->stats->last_baked_texture_count = resource_manager_get_baked_texture_count(renderer->lifecycle->resource_manager);
    renderer->stats->last_instanced_batch_count = 0U;
    renderer->stats->last_instanced_draw_count = 0U;
    renderer->stats->last_sort_ms = 0.0;
    renderer->stats->last_visibility_ms = 0.0;
    renderer->stats->last_body_draw_ms = 0.0;
    renderer->stats->last_overlay_draw_ms = 0.0;
    renderer->stats->last_instance_draw_ms = 0.0;
    resource_manager_begin_frame(renderer->lifecycle->resource_manager);

    if (frame->draw_scene_renderables) {
        double started_ms;
        draw_material_renderables = frame->material_renderables;
        if (frame->backdrop_draw != NULL) {
            ENGINE_PROFILE_ZONE_BEGIN(backdrop_zone, "renderer_draw_backdrop");
            Target target;
            target_init(&target, backend, 0.0f, 0.0f);
            frame->backdrop_draw(&target, &renderer->lifecycle->camera, frame->backdrop_user_data);
            ENGINE_PROFILE_ZONE_END(backdrop_zone);
        }
        {
            ENGINE_PROFILE_ZONE_BEGIN(procedural_mesh_zone, "renderer_draw_procedural_meshes");
            for (index = 0U; index < frame->procedural_mesh_count; ++index) {
                renderer_draw_procedural_mesh(renderer, backend, frame, &frame->procedural_meshes[index]);
            }
            ENGINE_PROFILE_ZONE_END(procedural_mesh_zone);
        }
        if (draw_material_renderables != NULL && frame->material_renderable_count == 1U) {
            ENGINE_PROFILE_ZONE_BEGIN(single_renderable_zone, "renderer_draw_single_renderable");
            started_ms = renderer_now_ms();
            if (renderer_is_material_renderable_visible(renderer, &draw_material_renderables[0])) {
                renderer->stats->last_visible_material_renderable_count = 1U;
                renderer->stats->last_visibility_ms = renderer_now_ms() - started_ms;
                renderer_draw_material_renderable(renderer, backend, &draw_material_renderables[0], frame->now_ms);
                renderer->stats->last_material_renderable_draw_count = 1U;
            } else {
                renderer->stats->last_visibility_ms = renderer_now_ms() - started_ms;
            }
            if (resource_manager_has_pending_bakes(renderer->lifecycle->resource_manager)) {
                ENGINE_PROFILE_ZONE_BEGIN(pending_bakes_single_zone, "renderer_process_pending_bakes");
                resource_manager_process_pending_bakes(
                    renderer->lifecycle->resource_manager,
                    renderer_compute_bake_budget_ms(frame_started_ms, renderer->lifecycle->prebake_target_fps)
                );
                ENGINE_PROFILE_ZONE_END(pending_bakes_single_zone);
            }
            ENGINE_PROFILE_ZONE_END(single_renderable_zone);
            goto draw_debug;
        }
        if (!material_renderables_sorted_by_layer && frame->material_renderables != NULL && frame->material_renderable_count > 0U) {
            for (index = 1U; index < frame->material_renderable_count; ++index) {
                if (frame->material_renderables[index - 1U].layer != frame->material_renderables[index].layer) {
                    needs_sort = true;
                    break;
                }
            }
        }
        if (frame->material_renderables != NULL && needs_sort && renderer_reserve_scratch(renderer, frame->material_renderable_count)) {
            ENGINE_PROFILE_ZONE_BEGIN(sort_zone, "renderer_sort_material_renderables");
            started_ms = renderer_now_ms();
            memcpy(renderer->scratch->scratch_material_renderables, frame->material_renderables, frame->material_renderable_count * sizeof(*renderer->scratch->scratch_material_renderables));
            qsort(renderer->scratch->scratch_material_renderables, frame->material_renderable_count, sizeof(*renderer->scratch->scratch_material_renderables), renderer_compare_material_layer);
            renderer->stats->last_sort_ms = renderer_now_ms() - started_ms;
            draw_material_renderables = renderer->scratch->scratch_material_renderables;
            ENGINE_PROFILE_ZONE_END(sort_zone);
        }
        if (draw_material_renderables != NULL) {
            ENGINE_PROFILE_ZONE_BEGIN(material_draw_zone, "renderer_draw_material_renderables");
            RendererTextureBatch batch = { 0 };

            batching_enabled = renderer_reserve_instances(renderer, frame->material_renderable_count);
            for (index = 0U; index < frame->material_renderable_count; ++index) {
                RendererPreparedTextureDraw prepared;
                double started_ms;

                started_ms = renderer_now_ms();
                if (!renderer_is_material_renderable_visible(renderer, &draw_material_renderables[index])) {
                    renderer->stats->last_visibility_ms += renderer_now_ms() - started_ms;
                    continue;
                }
                renderer->stats->last_visibility_ms += renderer_now_ms() - started_ms;
                renderer->stats->last_visible_material_renderable_count += 1U;

                if (batching_enabled &&
                    renderer_prepare_batched_material_draw(renderer, &draw_material_renderables[index], frame->now_ms, &prepared)) {
                    if (batch.texture != NULL &&
                        prepared.texture != batch.texture) {
                        renderer_flush_texture_batch(renderer, backend, &batch);
                    }
                    batch.texture = prepared.texture;
                    batch.tint = prepared.instance.tint;
                    renderer->scratch->scratch_instances[batch.count++] = prepared.instance;
                    continue;
                }

                renderer_flush_texture_batch(renderer, backend, &batch);
                renderer_draw_material_renderable(renderer, backend, &draw_material_renderables[index], frame->now_ms);
                renderer->stats->last_material_renderable_draw_count += 1U;
            }
            renderer_flush_texture_batch(renderer, backend, &batch);
            ENGINE_PROFILE_ZONE_END(material_draw_zone);
        }
        if (resource_manager_has_pending_bakes(renderer->lifecycle->resource_manager)) {
            ENGINE_PROFILE_ZONE_BEGIN(pending_bakes_frame_zone, "renderer_process_pending_bakes");
            resource_manager_process_pending_bakes(
                renderer->lifecycle->resource_manager,
                renderer_compute_bake_budget_ms(frame_started_ms, renderer->lifecycle->prebake_target_fps)
            );
            ENGINE_PROFILE_ZONE_END(pending_bakes_frame_zone);
        }
    }

draw_debug:
    if (frame->draw_debug) {
        ENGINE_PROFILE_ZONE_BEGIN(debug_world_zone, "renderer_draw_debug_world");
        debug_overlay_draw_world(
            renderer->lifecycle->debug_overlay,
            backend,
            frame->now_ms,
            &renderer->lifecycle->camera,
            &frame->debug_grid,
            frame->debug_entities,
            frame->debug_entity_count,
            frame->debug_collisions,
            frame->debug_collision_count,
            frame->selected_debug_entity_id,
            frame->hovered_debug_entity_id
        );
        ENGINE_PROFILE_ZONE_END(debug_world_zone);
    }
    renderer->signatures->last_frame_signature = frame_signature;
    renderer->signatures->last_sort_signature = sort_signature;
    renderer->signatures->last_overlay_signature = overlay_signature;
    renderer->signatures->last_instance_batch_signature = instance_signature;
    renderer->signatures->last_backdrop_signature = frame->backdrop_signature;
    renderer->signatures->last_metadata_signature = frame->metadata_signature;
    ENGINE_PROFILE_ZONE_END(renderer_draw_zone);
}
