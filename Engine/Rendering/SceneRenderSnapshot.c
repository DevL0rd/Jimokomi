#include "SceneRenderSnapshot.h"
#include "RenderSnapshotInternal.h"

#include "../Physics/PhysicsBodyControl.h"
#include "../Physics/PhysicsParticles.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneParticleVisualsInternal.h"
#include "../Scene/ScenePhysics.h"
#include "../Scene/SceneQueries.h"
#include "../Scene/SceneStats.h"
#include "../Scene/SceneView.h"
#include "../Scene/Entity.h"
#include "../Scene/Components/ColliderComponent.h"
#include "../Scene/Components/DraggableComponent.h"
#include "../Scene/Components/RenderableComponent.h"
#include "../Scene/Components/RigidBodyComponent.h"
#include "../Scene/Components/SelectableComponent.h"
#include "../Scene/Components/TransformComponent.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SCENE_PARTICLE_MESH_DEFAULT_CELL_SIZE_RADIUS_SCALE 1.0f
#define SCENE_PARTICLE_MESH_DEFAULT_INFLUENCE_RADIUS_SCALE 1.85f
#define SCENE_PARTICLE_MESH_DEFAULT_THRESHOLD 0.58f

typedef struct SceneRenderSnapshotDesc
{
    struct Scene* scene;
    const TaskSystem* task_system;
    SceneRenderSnapshotBuilder* builder;
    struct Entity** scratch_entities;
    size_t scratch_entity_capacity;
    PhysicsParticleRenderData* scratch_particles;
    size_t scratch_particle_capacity;
    Camera camera;
    bool has_camera;
    Aabb view_bounds;
    float render_alpha;
    uint64_t now_ms;
    double update_ms;
    uint32_t physics_substeps;
    bool debug_overlay_enabled;
    bool draw_debug_world;
    uint64_t selected_entity_id;
    uint64_t hovered_entity_id;
    RendererBackdropDrawFn backdrop_draw;
    void* backdrop_user_data;
    uint64_t backdrop_signature;
    const PhysicsWorldSnapshot* physics_snapshot;
} SceneRenderSnapshotDesc;

struct SceneRenderSnapshotBuilder {
    Entity** scratch_entities;
    size_t scratch_entity_capacity;
    PhysicsParticleRenderData* scratch_particles;
    size_t scratch_particle_capacity;
    float* scratch_mesh_values;
    float* scratch_mesh_red;
    float* scratch_mesh_green;
    float* scratch_mesh_blue;
    float* scratch_mesh_alpha;
    size_t scratch_mesh_capacity;
    float* scratch_worker_mesh_values;
    float* scratch_worker_mesh_red;
    float* scratch_worker_mesh_green;
    float* scratch_worker_mesh_blue;
    float* scratch_worker_mesh_alpha;
    size_t scratch_worker_mesh_capacity;
};

static bool scene_render_snapshot_builder_reserve_entities(
    SceneRenderSnapshotBuilder* builder,
    size_t required_capacity
) {
    Entity** next_entities;
    size_t next_capacity;

    if (builder == NULL) {
        return false;
    }
    if (builder->scratch_entity_capacity >= required_capacity) {
        return true;
    }

    next_capacity = builder->scratch_entity_capacity > 0U
        ? builder->scratch_entity_capacity
        : 64U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_entities = (Entity**)realloc(
        builder->scratch_entities,
        next_capacity * sizeof(*next_entities)
    );
    if (next_entities == NULL) {
        return false;
    }

    builder->scratch_entities = next_entities;
    builder->scratch_entity_capacity = next_capacity;
    return true;
}

SceneRenderSnapshotBuilder* scene_render_snapshot_builder_create(void) {
    return (SceneRenderSnapshotBuilder*)calloc(1U, sizeof(SceneRenderSnapshotBuilder));
}

void scene_render_snapshot_builder_destroy(SceneRenderSnapshotBuilder* builder) {
    if (builder == NULL) {
        return;
    }

    free(builder->scratch_entities);
    free(builder->scratch_particles);
    free(builder->scratch_mesh_values);
    free(builder->scratch_mesh_red);
    free(builder->scratch_mesh_green);
    free(builder->scratch_mesh_blue);
    free(builder->scratch_mesh_alpha);
    free(builder->scratch_worker_mesh_values);
    free(builder->scratch_worker_mesh_red);
    free(builder->scratch_worker_mesh_green);
    free(builder->scratch_worker_mesh_blue);
    free(builder->scratch_worker_mesh_alpha);
    free(builder);
}

static bool scene_render_snapshot_builder_reserve_particles(
    SceneRenderSnapshotBuilder* builder,
    size_t required_capacity
) {
    PhysicsParticleRenderData* next_particles;
    size_t next_capacity;

    if (builder == NULL) {
        return false;
    }
    if (builder->scratch_particle_capacity >= required_capacity) {
        return true;
    }

    next_capacity = builder->scratch_particle_capacity > 0U
        ? builder->scratch_particle_capacity
        : 256U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_particles = (PhysicsParticleRenderData*)realloc(
        builder->scratch_particles,
        next_capacity * sizeof(*next_particles)
    );
    if (next_particles == NULL) {
        return false;
    }

    builder->scratch_particles = next_particles;
    builder->scratch_particle_capacity = next_capacity;
    return true;
}

static bool scene_render_snapshot_builder_reserve_mesh_field(
    SceneRenderSnapshotBuilder* builder,
    size_t required_capacity
) {
    float* next_values;
    float* next_red;
    float* next_green;
    float* next_blue;
    float* next_alpha;
    size_t next_capacity;

    if (builder == NULL) {
        return false;
    }
    if (builder->scratch_mesh_capacity >= required_capacity) {
        return true;
    }

    next_capacity = builder->scratch_mesh_capacity > 0U
        ? builder->scratch_mesh_capacity
        : 4096U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_values = (float*)malloc(next_capacity * sizeof(*next_values));
    next_red = (float*)malloc(next_capacity * sizeof(*next_red));
    next_green = (float*)malloc(next_capacity * sizeof(*next_green));
    next_blue = (float*)malloc(next_capacity * sizeof(*next_blue));
    next_alpha = (float*)malloc(next_capacity * sizeof(*next_alpha));
    if (next_values == NULL || next_red == NULL || next_green == NULL || next_blue == NULL || next_alpha == NULL) {
        free(next_values);
        free(next_red);
        free(next_green);
        free(next_blue);
        free(next_alpha);
        return false;
    }

    free(builder->scratch_mesh_values);
    free(builder->scratch_mesh_red);
    free(builder->scratch_mesh_green);
    free(builder->scratch_mesh_blue);
    free(builder->scratch_mesh_alpha);
    builder->scratch_mesh_values = next_values;
    builder->scratch_mesh_red = next_red;
    builder->scratch_mesh_green = next_green;
    builder->scratch_mesh_blue = next_blue;
    builder->scratch_mesh_alpha = next_alpha;
    builder->scratch_mesh_capacity = next_capacity;
    return true;
}

static bool scene_render_snapshot_builder_reserve_worker_mesh_field(
    SceneRenderSnapshotBuilder* builder,
    size_t required_capacity
) {
    float* next_values;
    float* next_red;
    float* next_green;
    float* next_blue;
    float* next_alpha;
    size_t next_capacity;

    if (builder == NULL)
    {
        return false;
    }
    if (builder->scratch_worker_mesh_capacity >= required_capacity)
    {
        return true;
    }

    next_capacity = builder->scratch_worker_mesh_capacity > 0U
        ? builder->scratch_worker_mesh_capacity
        : 4096U;
    while (next_capacity < required_capacity)
    {
        next_capacity *= 2U;
    }

    next_values = (float*)malloc(next_capacity * sizeof(*next_values));
    next_red = (float*)malloc(next_capacity * sizeof(*next_red));
    next_green = (float*)malloc(next_capacity * sizeof(*next_green));
    next_blue = (float*)malloc(next_capacity * sizeof(*next_blue));
    next_alpha = (float*)malloc(next_capacity * sizeof(*next_alpha));
    if (next_values == NULL || next_red == NULL || next_green == NULL || next_blue == NULL || next_alpha == NULL)
    {
        free(next_values);
        free(next_red);
        free(next_green);
        free(next_blue);
        free(next_alpha);
        return false;
    }

    free(builder->scratch_worker_mesh_values);
    free(builder->scratch_worker_mesh_red);
    free(builder->scratch_worker_mesh_green);
    free(builder->scratch_worker_mesh_blue);
    free(builder->scratch_worker_mesh_alpha);
    builder->scratch_worker_mesh_values = next_values;
    builder->scratch_worker_mesh_red = next_red;
    builder->scratch_worker_mesh_green = next_green;
    builder->scratch_worker_mesh_blue = next_blue;
    builder->scratch_worker_mesh_alpha = next_alpha;
    builder->scratch_worker_mesh_capacity = next_capacity;
    return true;
}

static float scene_render_snapshot_lerp(float a, float b, float alpha)
{
    return a + ((b - a) * alpha);
}

static float scene_render_snapshot_lerp_angle(float a, float b, float alpha)
{
    float delta = fmodf(b - a, 6.28318530718f);
    if (delta > 3.14159265359f)
    {
        delta -= 6.28318530718f;
    }
    else if (delta < -3.14159265359f)
    {
        delta += 6.28318530718f;
    }
    return a + delta * alpha;
}

static uint64_t scene_render_snapshot_hash_u64(uint64_t hash, uint64_t value)
{
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
    return hash;
}

Aabb scene_render_snapshot_compute_view_bounds(
    const struct Scene* scene,
    const Camera* camera,
    float preload_screens
)
{
    Aabb bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    SceneView scene_view;

    if (scene == NULL)
    {
        return bounds;
    }

    scene_view = Scene_GetViewSnapshot(scene);
    bounds.min_x = scene_view.has_world_bounds ? scene_view.world_min_x : 0.0f;
    bounds.min_y = scene_view.has_world_bounds ? scene_view.world_min_y : 0.0f;
    bounds.max_x = scene_view.has_world_bounds ? scene_view.world_max_x : scene_view.view_width;
    bounds.max_y = scene_view.has_world_bounds ? scene_view.world_max_y : scene_view.view_height;
    if (camera != NULL && camera->view_width > 0.0f && camera->view_height > 0.0f)
    {
        float preload_x = camera->view_width * preload_screens;
        float preload_y = camera->view_height * preload_screens;

        bounds.min_x = camera->x - preload_x;
        bounds.min_y = camera->y - preload_y;
        bounds.max_x = camera->x + camera->view_width + preload_x;
        bounds.max_y = camera->y + camera->view_height + preload_y;
        if (scene_view.has_world_bounds)
        {
            bounds.min_x = clamp_f(bounds.min_x, scene_view.world_min_x, scene_view.world_max_x);
            bounds.min_y = clamp_f(bounds.min_y, scene_view.world_min_y, scene_view.world_max_y);
            bounds.max_x = clamp_f(bounds.max_x, scene_view.world_min_x, scene_view.world_max_x);
            bounds.max_y = clamp_f(bounds.max_y, scene_view.world_min_y, scene_view.world_max_y);
        }
    }

    return bounds;
}

static size_t scene_render_snapshot_collect_frame_data(
    const SceneRenderSnapshotDesc* desc,
    MaterialRenderable* material_renderables,
    DebugEntityView* entities,
    size_t material_renderable_capacity,
    uint64_t* out_material_frame_signature,
    uint64_t* out_material_sort_signature,
    uint64_t* out_material_instance_signature,
    bool* out_material_renderables_sorted_by_layer,
    DebugEntityView* out_selected_entity,
    bool* out_has_selected_entity,
    DebugEntityView* out_hovered_entity,
    bool* out_has_hovered_entity,
    PickTargetView* pick_targets,
    size_t pick_target_capacity,
    size_t* out_pick_target_count
)
{
    size_t index;
    size_t visible_count = 0U;
    size_t candidate_count;
    size_t pick_target_count = 0U;
    uint64_t material_frame_signature = 1469598103934665603ULL;
    uint64_t material_sort_signature = 1099511628211ULL;
    uint64_t material_instance_signature = 88172645463325252ULL;
    bool collect_debug;

    if (desc == NULL || desc->scene == NULL || material_renderables == NULL || desc->scratch_entities == NULL)
    {
        return 0U;
    }

    if (out_material_frame_signature != NULL) *out_material_frame_signature = material_frame_signature;
    if (out_material_sort_signature != NULL) *out_material_sort_signature = material_sort_signature;
    if (out_material_instance_signature != NULL) *out_material_instance_signature = material_instance_signature;
    if (out_material_renderables_sorted_by_layer != NULL) *out_material_renderables_sorted_by_layer = true;
    if (out_has_selected_entity != NULL) *out_has_selected_entity = false;
    if (out_has_hovered_entity != NULL) *out_has_hovered_entity = false;
    if (out_pick_target_count != NULL) *out_pick_target_count = 0U;

    candidate_count = Scene_QueryEntitiesInAabb(
        desc->scene,
        desc->view_bounds,
        desc->scratch_entities,
        desc->scratch_entity_capacity
    );

    collect_debug = entities != NULL && desc->debug_overlay_enabled && desc->draw_debug_world;
    for (index = 0U; index < candidate_count && visible_count < material_renderable_capacity; ++index)
    {
        Entity* entity = desc->scratch_entities[index];
        TransformComponent* transform;
        RenderableComponent* renderable;
        ColliderComponent* collider;
        MaterialRenderable* item;
        uint64_t entity_id;

        if (entity == NULL || !Entity_IsActive(entity))
        {
            continue;
        }

        transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
        renderable = (RenderableComponent*)Entity_GetComponent(entity, COMPONENT_RENDERABLE);
        if (transform == NULL || renderable == NULL || !renderable->visible)
        {
            continue;
        }

        item = &material_renderables[visible_count];
        item->x = scene_render_snapshot_lerp(transform->previous_x, transform->x, desc->render_alpha);
        item->y = scene_render_snapshot_lerp(transform->previous_y, transform->y, desc->render_alpha);
        item->angle_radians = scene_render_snapshot_lerp_angle(
            transform->previous_angle_radians,
            transform->angle_radians,
            desc->render_alpha
        );
        item->anchor_x = renderable->anchor_x;
        item->anchor_y = renderable->anchor_y;
        item->layer = renderable->layer;
        item->visible = renderable->visible;
        item->material_handle = renderable->material_handle;
        item->tint = renderable->tint.value != 0U ? renderable->tint : (Color32){ 0xffffffffU };
        item->user_data = NULL;

        entity_id = (uint64_t)Entity_GetId(entity);
        material_frame_signature = scene_render_snapshot_hash_u64(material_frame_signature, (uint64_t)(int64_t)item->layer);
        material_frame_signature = scene_render_snapshot_hash_u64(material_frame_signature, item->visible ? 1U : 0U);
        material_frame_signature = scene_render_snapshot_hash_u64(material_frame_signature, item->material_handle.id);
        material_frame_signature = scene_render_snapshot_hash_u64(material_frame_signature, item->tint.value);
        material_instance_signature = scene_render_snapshot_hash_u64(material_instance_signature, item->material_handle.id);
        material_instance_signature = scene_render_snapshot_hash_u64(material_instance_signature, item->tint.value);

        collider = (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
        if (pick_targets != NULL && pick_target_count < pick_target_capacity && collider != NULL)
        {
            DraggableComponent* draggable = (DraggableComponent*)Entity_GetComponent(entity, COMPONENT_DRAGGABLE);
            if (draggable != NULL && draggable->enabled)
            {
                PickTargetView* target = &pick_targets[pick_target_count++];

                target->id = entity_id;
                target->x = item->x;
                target->y = item->y;
                target->radius = draggable->pick_radius;
                target->extent_x = collider->shape == COLLIDER_SHAPE_CIRCLE
                    ? draggable->pick_radius
                    : collider->width * 0.5f;
                target->extent_y = collider->shape == COLLIDER_SHAPE_CIRCLE
                    ? draggable->pick_radius
                    : collider->height * 0.5f;
                target->is_circle = collider->shape == COLLIDER_SHAPE_CIRCLE;
            }
        }

        if (collider != NULL &&
            (collect_debug || entity_id == desc->selected_entity_id || entity_id == desc->hovered_entity_id))
        {
            DebugEntityView debug_entity = { 0 };
            RigidBodyComponent* rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
            SelectableComponent* selectable = (SelectableComponent*)Entity_GetComponent(entity, COMPONENT_SELECTABLE);

            debug_entity.id = entity_id;
            debug_entity.position.x = item->x;
            debug_entity.position.y = item->y;
            debug_entity.angle_radians = item->angle_radians;
            debug_entity.extent_x = collider->shape == COLLIDER_SHAPE_CIRCLE ? collider->radius : collider->width * 0.5f;
            debug_entity.extent_y = collider->shape == COLLIDER_SHAPE_CIRCLE ? collider->radius : collider->height * 0.5f;
            debug_entity.radius = collider->radius;
            debug_entity.is_circle = collider->shape == COLLIDER_SHAPE_CIRCLE;
            debug_entity.is_selected = selectable != NULL
                ? selectable->selected
                : entity_id == desc->selected_entity_id;
            if (rigid_body != NULL && rigid_body->has_body && Scene_GetPhysicsWorld(desc->scene) != NULL)
            {
                Vec2 velocity = { 0.0f, 0.0f };
                bool awake = false;
                PhysicsWorld_GetEntityLinearVelocity(Scene_GetPhysicsWorld(desc->scene), entity, &velocity);
                PhysicsWorld_IsEntityAwake(Scene_GetPhysicsWorld(desc->scene), entity, &awake);
                debug_entity.velocity = velocity;
                debug_entity.is_sleeping = !awake;
                debug_entity.is_moving = fabsf(velocity.x) + fabsf(velocity.y) > 0.05f;
            }
            if (collect_debug)
            {
                entities[visible_count] = debug_entity;
            }
            if (out_selected_entity != NULL && out_has_selected_entity != NULL &&
                !*out_has_selected_entity && entity_id == desc->selected_entity_id)
            {
                *out_selected_entity = debug_entity;
                *out_has_selected_entity = true;
            }
            if (out_hovered_entity != NULL && out_has_hovered_entity != NULL &&
                !*out_has_hovered_entity && entity_id == desc->hovered_entity_id)
            {
                *out_hovered_entity = debug_entity;
                *out_has_hovered_entity = true;
            }
        }

        visible_count += 1U;
    }

    if (out_material_frame_signature != NULL) *out_material_frame_signature = material_frame_signature;
    if (out_material_sort_signature != NULL) *out_material_sort_signature = scene_render_snapshot_hash_u64(material_sort_signature, visible_count);
    if (out_material_instance_signature != NULL) *out_material_instance_signature = material_instance_signature;
    if (out_material_renderables_sorted_by_layer != NULL) *out_material_renderables_sorted_by_layer = true;
    if (out_pick_target_count != NULL) *out_pick_target_count = pick_target_count;
    return visible_count;
}

static size_t scene_render_snapshot_get_particle_visual_count(const Scene* scene)
{
    const SceneParticleVisualDesc* bindings = NULL;
    const PhysicsWorld* physics_world = NULL;
    size_t binding_count = 0U;
    size_t index = 0U;
    size_t material_renderable_count = 0U;

    if (scene == NULL)
    {
        return 0U;
    }

    physics_world = Scene_GetPhysicsWorldConst(scene);
    bindings = Scene_GetParticleVisualBindings(scene, &binding_count);
    if (physics_world == NULL || bindings == NULL)
    {
        return 0U;
    }

    for (index = 0U; index < binding_count; ++index)
    {
        if (!bindings[index].particles_visible)
        {
            continue;
        }
        material_renderable_count += PhysicsWorld_GetParticleSystemParticleCount(physics_world, bindings[index].particle_system);
    }

    return material_renderable_count;
}

static Aabb scene_render_snapshot_expand_aabb(Aabb bounds, float amount)
{
    if (amount <= 0.0f)
    {
        return bounds;
    }

    return (Aabb){
        bounds.min_x - amount,
        bounds.min_y - amount,
        bounds.max_x + amount,
        bounds.max_y + amount
    };
}

static size_t scene_render_snapshot_append_particle_visuals(
    const SceneRenderSnapshotDesc* desc,
    PhysicsParticleRenderData* scratch_particles,
    size_t scratch_particle_capacity,
    MaterialRenderable* material_renderables,
    size_t material_renderable_capacity,
    size_t material_renderable_count,
    uint64_t* material_frame_signature,
    uint64_t* material_sort_signature,
    uint64_t* material_instance_signature
)
{
    const SceneParticleVisualDesc* bindings = NULL;
    const PhysicsWorld* physics_world = NULL;
    size_t binding_count = 0U;
    size_t binding_index = 0U;

    if (desc == NULL || desc->scene == NULL || scratch_particles == NULL || material_renderables == NULL)
    {
        return material_renderable_count;
    }

    physics_world = Scene_GetPhysicsWorldConst(desc->scene);
    bindings = Scene_GetParticleVisualBindings(desc->scene, &binding_count);
    if (physics_world == NULL || bindings == NULL)
    {
        return material_renderable_count;
    }

    for (binding_index = 0U; binding_index < binding_count && material_renderable_count < material_renderable_capacity; ++binding_index)
    {
        const SceneParticleVisualDesc* binding = &bindings[binding_index];
        size_t capacity = material_renderable_capacity - material_renderable_count;
        size_t particle_count;
        size_t particle_index;
        float particle_radius;

        if (!binding->particles_visible || capacity == 0U)
        {
            continue;
        }

        if (capacity > scratch_particle_capacity)
        {
            capacity = scratch_particle_capacity;
        }
        particle_radius = PhysicsWorld_GetParticleSystemRadius(physics_world, binding->particle_system);
        particle_count = PhysicsWorld_CopyParticleSystemRenderDataInAabb(
            physics_world,
            binding->particle_system,
            scene_render_snapshot_expand_aabb(desc->view_bounds, particle_radius),
            scratch_particles,
            capacity
        );

        if (material_frame_signature != NULL)
        {
            *material_frame_signature = scene_render_snapshot_hash_u64(*material_frame_signature, particle_count);
            *material_frame_signature = scene_render_snapshot_hash_u64(*material_frame_signature, binding->particle_material_handle.id);
        }

        for (particle_index = 0U; particle_index < particle_count && material_renderable_count < material_renderable_capacity; ++particle_index)
        {
            const PhysicsParticleRenderData* particle = &scratch_particles[particle_index];
            MaterialRenderable* item = &material_renderables[material_renderable_count++];
            Color32 tint = (Color32){
                particle->color_argb != 0U
                    ? particle->color_argb
                    : binding->particle_fallback_tint.value
            };

            item->x = particle->position.x;
            item->y = particle->position.y;
            item->angle_radians = 0.0f;
            item->anchor_x = 0.5f;
            item->anchor_y = 0.5f;
            item->layer = binding->particle_layer;
            item->visible = true;
            item->material_handle = binding->particle_material_handle;
            item->tint = tint;
            item->user_data = NULL;

            if (material_sort_signature != NULL)
            {
                *material_sort_signature = scene_render_snapshot_hash_u64(*material_sort_signature, (uint64_t)(int64_t)item->layer);
            }
            if (material_instance_signature != NULL)
            {
                *material_instance_signature = scene_render_snapshot_hash_u64(*material_instance_signature, item->material_handle.id);
                *material_instance_signature = scene_render_snapshot_hash_u64(*material_instance_signature, item->tint.value);
            }
        }
    }

    return material_renderable_count;
}

static uint8_t scene_render_snapshot_color_channel(uint32_t argb, uint32_t shift)
{
    return (uint8_t)((argb >> shift) & 0xffU);
}

static Vec2 scene_render_snapshot_interp_mesh_point(
    Vec2 a,
    Vec2 b,
    float value_a,
    float value_b,
    float threshold
) {
    float span = value_b - value_a;
    float t = fabsf(span) > 0.000001f ? (threshold - value_a) / span : 0.5f;
    t = clamp_f(t, 0.0f, 1.0f);
    return (Vec2){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}

static Color32 scene_render_snapshot_mesh_color(
    const SceneParticleVisualDesc* binding,
    const float* values,
    const float* red,
    const float* green,
    const float* blue,
    const float* alpha,
    size_t a,
    size_t b,
    size_t c,
    size_t d
) {
    float total = values[a] + values[b] + values[c] + values[d];
    float tint_alpha = (float)scene_render_snapshot_color_channel(binding->mesh_tint.value, 24U);
    float r;
    float g;
    float bl;
    float al;

    if (total <= 0.000001f)
    {
        return binding->mesh_tint;
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
        (uint8_t)clamp_f(r, 0.0f, 255.0f),
        (uint8_t)clamp_f(g, 0.0f, 255.0f),
        (uint8_t)clamp_f(bl, 0.0f, 255.0f),
        (uint8_t)clamp_f(al, 0.0f, 255.0f)
    );
}

typedef struct SceneParticleMeshFieldContext
{
    const PhysicsParticleRenderData* particles;
    size_t particle_count;
    uint32_t fallback_color;
    float min_x;
    float min_y;
    float cell_size;
    float influence_radius;
    size_t vertex_count_x;
    size_t vertex_count_y;
    size_t vertex_count;
    size_t worker_count;
    float* values;
    float* red;
    float* green;
    float* blue;
    float* alpha;
} SceneParticleMeshFieldContext;

static void scene_render_snapshot_accumulate_mesh_field_range(
    int start_index,
    int end_index,
    uint32_t worker_index,
    void* user_context
) {
    SceneParticleMeshFieldContext* context = (SceneParticleMeshFieldContext*)user_context;
    size_t worker_offset;
    int particle_index;

    if (context == NULL ||
        context->particles == NULL ||
        worker_index >= context->worker_count)
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
        int min_vertex_x = (int)floorf((particle->position.x - context->influence_radius - context->min_x) / context->cell_size);
        int min_vertex_y = (int)floorf((particle->position.y - context->influence_radius - context->min_y) / context->cell_size);
        int max_vertex_x = (int)ceilf((particle->position.x + context->influence_radius - context->min_x) / context->cell_size);
        int max_vertex_y = (int)ceilf((particle->position.y + context->influence_radius - context->min_y) / context->cell_size);
        int vertex_y;

        if (min_vertex_x < 0) min_vertex_x = 0;
        if (min_vertex_y < 0) min_vertex_y = 0;
        if (max_vertex_x >= (int)context->vertex_count_x) max_vertex_x = (int)context->vertex_count_x - 1;
        if (max_vertex_y >= (int)context->vertex_count_y) max_vertex_y = (int)context->vertex_count_y - 1;

        for (vertex_y = min_vertex_y; vertex_y <= max_vertex_y; ++vertex_y)
        {
            int vertex_x;
            float y = context->min_y + (float)vertex_y * context->cell_size;
            for (vertex_x = min_vertex_x; vertex_x <= max_vertex_x; ++vertex_x)
            {
                float x = context->min_x + (float)vertex_x * context->cell_size;
                float dx = x - particle->position.x;
                float dy = y - particle->position.y;
                float distance = sqrtf(dx * dx + dy * dy);
                float weight = 1.0f - (distance / context->influence_radius);
                if (weight > 0.0f)
                {
                    size_t vertex_index = worker_offset + (size_t)vertex_y * context->vertex_count_x + (size_t)vertex_x;
                    context->values[vertex_index] += weight;
                    context->red[vertex_index] += weight * (float)scene_render_snapshot_color_channel(color, 16U);
                    context->green[vertex_index] += weight * (float)scene_render_snapshot_color_channel(color, 8U);
                    context->blue[vertex_index] += weight * (float)scene_render_snapshot_color_channel(color, 0U);
                    context->alpha[vertex_index] += weight * (float)scene_render_snapshot_color_channel(color, 24U);
                }
            }
        }
    }
}

typedef struct SceneParticleMeshReduceContext
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
} SceneParticleMeshReduceContext;

static void scene_render_snapshot_reduce_mesh_field_range(
    int start_index,
    int end_index,
    uint32_t worker_index,
    void* user_context
) {
    SceneParticleMeshReduceContext* context = (SceneParticleMeshReduceContext*)user_context;
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

static bool scene_render_snapshot_add_mesh_triangle(
    RenderWorldSnapshot* world,
    const Camera* camera,
    Vec2 a,
    Vec2 b,
    Vec2 c,
    Color32 color,
    int layer
) {
    TriangleRenderable* triangle;

    if (world == NULL)
    {
        return false;
    }
    if (!render_world_snapshot_reserve_triangles(world, world->triangle_count + 1U))
    {
        return false;
    }

    triangle = &world->triangles[world->triangle_count++];
    triangle->a = a;
    triangle->b = b;
    triangle->c = c;
    if (camera != NULL)
    {
        triangle->screen_a = camera_world_to_screen(camera, a);
        triangle->screen_b = camera_world_to_screen(camera, b);
        triangle->screen_c = camera_world_to_screen(camera, c);
        triangle->screen_space_valid = true;
    }
    else
    {
        triangle->screen_a = (Vec2){ 0.0f, 0.0f };
        triangle->screen_b = (Vec2){ 0.0f, 0.0f };
        triangle->screen_c = (Vec2){ 0.0f, 0.0f };
        triangle->screen_space_valid = false;
    }
    triangle->uv_a = a;
    triangle->uv_b = b;
    triangle->uv_c = c;
    triangle->color = color;
    triangle->layer = layer;
    triangle->visible = true;
    return true;
}

static void scene_render_snapshot_append_particle_mesh(
    const SceneRenderSnapshotDesc* desc,
    RenderWorldSnapshot* world
) {
    const SceneParticleVisualDesc* bindings = NULL;
    const PhysicsWorld* physics_world = NULL;
    size_t binding_count = 0U;
    size_t binding_index = 0U;

    if (desc == NULL || desc->scene == NULL || world == NULL || desc->scratch_particles == NULL)
    {
        return;
    }

    physics_world = Scene_GetPhysicsWorldConst(desc->scene);
    bindings = Scene_GetParticleVisualBindings(desc->scene, &binding_count);
    if (physics_world == NULL || bindings == NULL)
    {
        return;
    }

    for (binding_index = 0U; binding_index < binding_count; ++binding_index)
    {
        const SceneParticleVisualDesc* binding = &bindings[binding_index];
        size_t triangle_count_before = world->triangle_count;
        size_t line_count_before = world->line_count;
        ProceduralMeshRenderable* mesh = NULL;
        size_t particle_count;
        float min_x;
        float min_y;
        float max_x;
        float max_y;
        float cell_size;
        float influence_radius;
        float threshold;
        float particle_radius;
        size_t cell_count_x;
        size_t cell_count_y;
        size_t vertex_count_x;
        size_t vertex_count_y;
        size_t vertex_count;
        float* mesh_values;
        float* mesh_red;
        float* mesh_green;
        float* mesh_blue;
        float* mesh_alpha;
        size_t worker_count;
        size_t worker_field_capacity;
        size_t particle_index;
        size_t cell_y;

        if (!binding->mesh_visible ||
            binding->mesh_handle.id == 0U)
        {
            continue;
        }

        particle_radius = PhysicsWorld_GetParticleSystemRadius(physics_world, binding->particle_system);
        cell_size = binding->mesh_cell_size > 0.0f
            ? binding->mesh_cell_size
            : particle_radius * SCENE_PARTICLE_MESH_DEFAULT_CELL_SIZE_RADIUS_SCALE;
        influence_radius = binding->mesh_influence_radius > 0.0f
            ? binding->mesh_influence_radius
            : particle_radius * SCENE_PARTICLE_MESH_DEFAULT_INFLUENCE_RADIUS_SCALE;
        threshold = binding->mesh_threshold > 0.0f
            ? binding->mesh_threshold
            : SCENE_PARTICLE_MESH_DEFAULT_THRESHOLD;
        if (cell_size <= 0.0f || influence_radius <= 0.0f || threshold <= 0.0f)
        {
            continue;
        }

        particle_count = PhysicsWorld_CopyParticleSystemRenderDataInAabb(
            physics_world,
            binding->particle_system,
            scene_render_snapshot_expand_aabb(desc->view_bounds, influence_radius),
            desc->scratch_particles,
            desc->scratch_particle_capacity
        );
        if (particle_count == 0U)
        {
            continue;
        }

        if (!render_world_snapshot_reserve_procedural_meshes(world, world->procedural_mesh_count + 1U))
        {
            continue;
        }
        mesh = &world->procedural_meshes[world->procedural_mesh_count++];
        memset(mesh, 0, sizeof(*mesh));
        mesh->procedural_mesh_handle = binding->mesh_handle;
        mesh->material_handle = binding->mesh_material_handle;
        mesh->triangle_start = triangle_count_before;
        mesh->line_start = line_count_before;
        mesh->layer = binding->mesh_layer;
        mesh->visible = true;
        min_x = desc->scratch_particles[0].position.x - influence_radius;
        min_y = desc->scratch_particles[0].position.y - influence_radius;
        max_x = desc->scratch_particles[0].position.x + influence_radius;
        max_y = desc->scratch_particles[0].position.y + influence_radius;
        for (particle_index = 1U; particle_index < particle_count; ++particle_index)
        {
            const PhysicsParticleRenderData* particle = &desc->scratch_particles[particle_index];
            min_x = fminf(min_x, particle->position.x - influence_radius);
            min_y = fminf(min_y, particle->position.y - influence_radius);
            max_x = fmaxf(max_x, particle->position.x + influence_radius);
            max_y = fmaxf(max_y, particle->position.y + influence_radius);
        }

        cell_count_x = (size_t)ceilf((max_x - min_x) / cell_size);
        cell_count_y = (size_t)ceilf((max_y - min_y) / cell_size);
        if (cell_count_x == 0U || cell_count_y == 0U)
        {
            continue;
        }
        if (!render_world_snapshot_reserve_triangles(
                world,
                world->triangle_count + cell_count_x * cell_count_y * 6U))
        {
            continue;
        }
        vertex_count_x = cell_count_x + 1U;
        vertex_count_y = cell_count_y + 1U;
        vertex_count = vertex_count_x * vertex_count_y;
        if (!scene_render_snapshot_builder_reserve_mesh_field(desc->builder, vertex_count))
        {
            continue;
        }
        mesh_values = desc->builder->scratch_mesh_values;
        mesh_red = desc->builder->scratch_mesh_red;
        mesh_green = desc->builder->scratch_mesh_green;
        mesh_blue = desc->builder->scratch_mesh_blue;
        mesh_alpha = desc->builder->scratch_mesh_alpha;

        worker_count = desc->task_system != NULL
            ? (size_t)task_system_get_worker_count(desc->task_system)
            : 1U;
        if (worker_count < 1U)
        {
            worker_count = 1U;
        }
        worker_field_capacity = vertex_count * worker_count;
        if (!scene_render_snapshot_builder_reserve_worker_mesh_field(desc->builder, worker_field_capacity))
        {
            continue;
        }

        memset(desc->builder->scratch_worker_mesh_values, 0, worker_field_capacity * sizeof(*desc->builder->scratch_worker_mesh_values));
        memset(desc->builder->scratch_worker_mesh_red, 0, worker_field_capacity * sizeof(*desc->builder->scratch_worker_mesh_red));
        memset(desc->builder->scratch_worker_mesh_green, 0, worker_field_capacity * sizeof(*desc->builder->scratch_worker_mesh_green));
        memset(desc->builder->scratch_worker_mesh_blue, 0, worker_field_capacity * sizeof(*desc->builder->scratch_worker_mesh_blue));
        memset(desc->builder->scratch_worker_mesh_alpha, 0, worker_field_capacity * sizeof(*desc->builder->scratch_worker_mesh_alpha));

        {
            SceneParticleMeshFieldContext field_context = {
                .particles = desc->scratch_particles,
                .particle_count = particle_count,
                .fallback_color = binding->particle_fallback_tint.value,
                .min_x = min_x,
                .min_y = min_y,
                .cell_size = cell_size,
                .influence_radius = influence_radius,
                .vertex_count_x = vertex_count_x,
                .vertex_count_y = vertex_count_y,
                .vertex_count = vertex_count,
                .worker_count = worker_count,
                .values = desc->builder->scratch_worker_mesh_values,
                .red = desc->builder->scratch_worker_mesh_red,
                .green = desc->builder->scratch_worker_mesh_green,
                .blue = desc->builder->scratch_worker_mesh_blue,
                .alpha = desc->builder->scratch_worker_mesh_alpha
            };

            if (!task_system_parallel_for(
                    desc->task_system,
                    (int)particle_count,
                    1,
                    scene_render_snapshot_accumulate_mesh_field_range,
                    &field_context))
            {
                scene_render_snapshot_accumulate_mesh_field_range(
                    0,
                    (int)particle_count,
                    0U,
                    &field_context
                );
            }
        }

        {
            SceneParticleMeshReduceContext reduce_context = {
                .vertex_count = vertex_count,
                .worker_count = worker_count,
                .worker_values = desc->builder->scratch_worker_mesh_values,
                .worker_red = desc->builder->scratch_worker_mesh_red,
                .worker_green = desc->builder->scratch_worker_mesh_green,
                .worker_blue = desc->builder->scratch_worker_mesh_blue,
                .worker_alpha = desc->builder->scratch_worker_mesh_alpha,
                .values = mesh_values,
                .red = mesh_red,
                .green = mesh_green,
                .blue = mesh_blue,
                .alpha = mesh_alpha
            };

            if (!task_system_parallel_for(
                    desc->task_system,
                    (int)vertex_count,
                    1,
                    scene_render_snapshot_reduce_mesh_field_range,
                    &reduce_context))
            {
                scene_render_snapshot_reduce_mesh_field_range(
                    0,
                    (int)vertex_count,
                    0U,
                    &reduce_context
                );
            }
        }

        for (cell_y = 0U; cell_y < cell_count_y; ++cell_y)
        {
            size_t cell_x;
            for (cell_x = 0U; cell_x < cell_count_x; ++cell_x)
            {
                size_t v0 = cell_y * vertex_count_x + cell_x;
                size_t v1 = v0 + 1U;
                size_t v3 = v0 + vertex_count_x;
                size_t v2 = v3 + 1U;
                float value0 = mesh_values[v0];
                float value1 = mesh_values[v1];
                float value2 = mesh_values[v2];
                float value3 = mesh_values[v3];
                bool inside0 = value0 >= threshold;
                bool inside1 = value1 >= threshold;
                bool inside2 = value2 >= threshold;
                bool inside3 = value3 >= threshold;
                Vec2 p0 = { min_x + (float)cell_x * cell_size, min_y + (float)cell_y * cell_size };
                Vec2 p1 = { p0.x + cell_size, p0.y };
                Vec2 p2 = { p0.x + cell_size, p0.y + cell_size };
                Vec2 p3 = { p0.x, p0.y + cell_size };
                Vec2 points[8];
                size_t point_count = 0U;
                size_t point_index;
                Color32 color;

                if (!inside0 && !inside1 && !inside2 && !inside3)
                {
                    continue;
                }

                if (inside0) points[point_count++] = p0;
                if (inside0 != inside1) {
                    points[point_count++] = scene_render_snapshot_interp_mesh_point(p0, p1, value0, value1, threshold);
                }
                if (inside1) points[point_count++] = p1;
                if (inside1 != inside2) {
                    points[point_count++] = scene_render_snapshot_interp_mesh_point(p1, p2, value1, value2, threshold);
                }
                if (inside2) points[point_count++] = p2;
                if (inside2 != inside3) {
                    points[point_count++] = scene_render_snapshot_interp_mesh_point(p2, p3, value2, value3, threshold);
                }
                if (inside3) points[point_count++] = p3;
                if (inside3 != inside0) {
                    points[point_count++] = scene_render_snapshot_interp_mesh_point(p3, p0, value3, value0, threshold);
                }
                if (point_count < 3U)
                {
                    continue;
                }

                color = scene_render_snapshot_mesh_color(
                    binding,
                    mesh_values,
                    mesh_red,
                    mesh_green,
                    mesh_blue,
                    mesh_alpha,
                    v0,
                    v1,
                    v2,
                    v3
                );
                for (point_index = 1U; point_index + 1U < point_count; ++point_index)
                {
                    if (!scene_render_snapshot_add_mesh_triangle(
                        world,
                        desc->has_camera ? &desc->camera : NULL,
                        points[0],
                            points[point_index],
                            points[point_index + 1U],
                            color,
                            binding->mesh_layer))
                    {
                        return;
                    }
                }
            }
        }

        mesh->triangle_count = world->triangle_count - mesh->triangle_start;
        mesh->line_count = world->line_count - mesh->line_start;
        if (mesh->triangle_count == 0U && mesh->line_count == 0U)
        {
            world->procedural_mesh_count -= 1U;
        }

    }
}

static bool scene_render_snapshot_build_desc(
    const SceneRenderSnapshotDesc* desc,
    RenderSnapshotBuffer* buffer
)
{
    PhysicsWorldSnapshot empty_physics_snapshot;
    SceneStatsSnapshot scene_stats;
    SpatialGridCellSpan query_span;
    uint64_t snapshot_metadata_signature;
    const PhysicsWorldSnapshot* physics_snapshot;
    size_t entity_capacity;
    size_t particle_visual_capacity;
    size_t material_renderable_capacity;
    size_t contact_hit_capacity;
    size_t entity_item_count;

    if (desc == NULL || desc->scene == NULL || buffer == NULL)
    {
        return false;
    }

    memset(&empty_physics_snapshot, 0, sizeof(empty_physics_snapshot));
    physics_snapshot = desc->physics_snapshot != NULL ? desc->physics_snapshot : &empty_physics_snapshot;
    memset(&scene_stats, 0, sizeof(scene_stats));
    Scene_GetStatsSnapshot(desc->scene, &scene_stats);
    entity_capacity = Scene_GetEntityCount(desc->scene);
    particle_visual_capacity = scene_render_snapshot_get_particle_visual_count(desc->scene);
    material_renderable_capacity = entity_capacity + particle_visual_capacity;
    if (!render_world_snapshot_reserve_material_renderables(&buffer->world, material_renderable_capacity))
    {
        return false;
    }
    contact_hit_capacity = Scene_GetPhysicsWorld(desc->scene) != NULL
        ? PhysicsWorld_GetContactHitCount(Scene_GetPhysicsWorld(desc->scene))
        : 0U;
    if (!render_world_snapshot_reserve_debug_collisions(&buffer->world, contact_hit_capacity))
    {
        return false;
    }
    render_world_snapshot_reset(&buffer->world);
    memset(&buffer->stats, 0, sizeof(buffer->stats));

    buffer->world.backdrop_draw = desc->backdrop_draw;
    buffer->world.backdrop_user_data = desc->backdrop_user_data;
    buffer->world.backdrop_signature = desc->backdrop_signature;
    buffer->world.draw_scene_renderables = true;
    buffer->world.draw_debug_world = desc->debug_overlay_enabled && desc->draw_debug_world;
    buffer->world.now_ms = desc->now_ms;
    scene_render_snapshot_append_particle_mesh(desc, &buffer->world);
    entity_item_count = scene_render_snapshot_collect_frame_data(
        desc,
        buffer->world.material_renderables,
        buffer->world.debug_entities,
        buffer->world.material_renderable_capacity,
        &buffer->world.material_frame_signature,
        &buffer->world.material_sort_signature,
        &buffer->world.material_instance_signature,
        &buffer->world.material_renderables_sorted_by_layer,
        &buffer->world.selected_entity,
        &buffer->world.has_selected_entity,
        &buffer->world.hovered_entity,
        &buffer->world.has_hovered_entity,
        buffer->world.pick_targets,
        buffer->world.pick_target_capacity,
        &buffer->world.pick_target_count
    );
    buffer->world.material_renderable_count = scene_render_snapshot_append_particle_visuals(
        desc,
        desc->scratch_particles,
        desc->scratch_particle_capacity,
        buffer->world.material_renderables,
        buffer->world.material_renderable_capacity,
        entity_item_count,
        &buffer->world.material_frame_signature,
        &buffer->world.material_sort_signature,
        &buffer->world.material_instance_signature
    );
    if (buffer->world.material_renderable_count > entity_item_count)
    {
        buffer->world.material_renderables_sorted_by_layer = false;
        buffer->world.material_sort_signature = scene_render_snapshot_hash_u64(
            buffer->world.material_sort_signature,
            buffer->world.material_renderable_count
        );
    }
    buffer->world.material_signatures_valid = true;
    buffer->world.debug_entity_count = buffer->world.draw_debug_world ? entity_item_count : 0U;

    {
        SpatialGridStatsSnapshot spatial_stats;
        Scene_GetSpatialGridStatsSnapshot(desc->scene, &spatial_stats);
        if (spatial_stats.enabled)
        {
            buffer->world.debug_grid.enabled = true;
            buffer->world.debug_grid.min_x = spatial_stats.bounds.min_x;
            buffer->world.debug_grid.min_y = spatial_stats.bounds.min_y;
            buffer->world.debug_grid.max_x = spatial_stats.bounds.max_x;
            buffer->world.debug_grid.max_y = spatial_stats.bounds.max_y;
            buffer->world.debug_grid.cell_size = spatial_stats.cell_size;
        }
    }

    snapshot_metadata_signature = 1469598103934665603ULL;
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.material_renderable_count);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.debug_entity_count);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.draw_scene_renderables ? 1U : 0U);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.draw_debug_world ? 1U : 0U);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.backdrop_signature);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, desc->selected_entity_id);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, desc->hovered_entity_id);
    buffer->world.metadata_signature = snapshot_metadata_signature;
    buffer->world.metadata_dirty = true;

    buffer->stats.overlay.update_ms = (float)desc->update_ms;
    buffer->stats.overlay.sim_ms = (float)desc->update_ms;
    buffer->stats.overlay.physics_substeps = desc->physics_substeps;
    buffer->stats.overlay.visible_count = (uint32_t)Scene_GetEntityCount(desc->scene);
    buffer->stats.overlay.physics_hz = physics_snapshot->physics_hz;
    buffer->stats.overlay.physics_ms = physics_snapshot->corephys_step_wall_ms;
    buffer->stats.overlay.awake_body_count = physics_snapshot->body_count;
    buffer->stats.overlay.total_body_count = physics_snapshot->total_body_count;
    buffer->stats.overlay.sleeping_body_count = physics_snapshot->sleeping_body_count;
    buffer->stats.overlay.moved_body_count = physics_snapshot->moved_body_count;
    buffer->stats.overlay.particle_system_count = physics_snapshot->particle_system_count;
    buffer->stats.overlay.particle_count = physics_snapshot->particle_count;
    buffer->stats.overlay.particle_contact_count = physics_snapshot->particle_contact_count;
    buffer->stats.overlay.particle_body_contact_count = physics_snapshot->particle_body_contact_count;
    buffer->stats.overlay.particle_task_count = physics_snapshot->particle_task_count;
    buffer->stats.overlay.particle_task_range_count = physics_snapshot->particle_task_range_count;
    buffer->stats.overlay.particle_spatial_cell_count = physics_snapshot->particle_spatial_cell_count;
    buffer->stats.overlay.particle_occupied_cell_count = physics_snapshot->particle_occupied_cell_count;
    buffer->stats.overlay.particle_byte_count = physics_snapshot->particle_byte_count;
    buffer->stats.overlay.particle_scratch_byte_count = physics_snapshot->particle_scratch_byte_count;
    buffer->stats.overlay.particle_profile_ms = physics_snapshot->particle_profile_ms;
    buffer->stats.overlay.particle_profile_contacts_ms = physics_snapshot->particle_profile_contacts_ms;
    buffer->stats.overlay.particle_profile_body_contacts_ms = physics_snapshot->particle_profile_body_contacts_ms;
    buffer->stats.overlay.particle_profile_collision_ms = physics_snapshot->particle_profile_collision_ms;
    buffer->stats.overlay.physics_task_worker_count = physics_snapshot->task_worker_count;
    buffer->stats.overlay.physics_task_background_thread_count = physics_snapshot->task_background_thread_count;
    buffer->stats.overlay.physics_task_enqueued_count = physics_snapshot->corephys_enqueued_task_count;
    buffer->stats.overlay.physics_task_inline_count = physics_snapshot->corephys_inline_task_count;
    buffer->stats.overlay.physics_task_main_chunk_count = physics_snapshot->corephys_main_chunk_count;
    buffer->stats.overlay.physics_task_worker_chunk_count = physics_snapshot->corephys_worker_chunk_count;
    buffer->stats.overlay.render_alpha = desc->render_alpha;
    buffer->stats.render.render_alpha = desc->render_alpha;

    memset(&query_span, 0, sizeof(query_span));
    if (buffer->world.draw_debug_world &&
        Scene_GetSpatialGridCellSpanForAabb(desc->scene, desc->view_bounds, &query_span))
    {
        buffer->world.debug_grid.span_min_x = query_span.min_cell_x;
        buffer->world.debug_grid.span_max_x = query_span.max_cell_x;
        buffer->world.debug_grid.span_min_y = query_span.min_cell_y;
        buffer->world.debug_grid.span_max_y = query_span.max_cell_y;
    }

    buffer->stats.physics.physics_substeps = desc->physics_substeps;
    buffer->stats.physics.physics_hz = physics_snapshot->physics_hz;
    buffer->stats.physics.physics_step_substeps = physics_snapshot->physics_step_substeps;
    buffer->stats.physics.physics_active_entities = physics_snapshot->active_entity_count;
    buffer->stats.physics.physics_dirty_entities = physics_snapshot->dirty_entity_count;
    buffer->stats.physics.physics_collider_changed_entities = physics_snapshot->collider_changed_entity_count;
    buffer->stats.physics.physics_body_create_queue = physics_snapshot->body_create_count;
    buffer->stats.physics.physics_body_remove_queue = physics_snapshot->body_remove_count;
    buffer->stats.physics.physics_shape_change_queue = physics_snapshot->shape_change_count;
    buffer->stats.physics.particle_system_count = physics_snapshot->particle_system_count;
    buffer->stats.physics.particle_group_count = physics_snapshot->particle_group_count;
    buffer->stats.physics.particle_count = physics_snapshot->particle_count;
    buffer->stats.physics.particle_contact_count = physics_snapshot->particle_contact_count;
    buffer->stats.physics.particle_body_contact_count = physics_snapshot->particle_body_contact_count;
    buffer->stats.physics.particle_task_count = physics_snapshot->particle_task_count;
    buffer->stats.physics.particle_task_range_count = physics_snapshot->particle_task_range_count;
    buffer->stats.physics.particle_task_range_min = physics_snapshot->particle_task_range_min;
    buffer->stats.physics.particle_task_range_max = physics_snapshot->particle_task_range_max;
    buffer->stats.physics.particle_spatial_cell_count = physics_snapshot->particle_spatial_cell_count;
    buffer->stats.physics.particle_occupied_cell_count = physics_snapshot->particle_occupied_cell_count;
    buffer->stats.physics.particle_spatial_proxy_count = physics_snapshot->particle_spatial_proxy_count;
    buffer->stats.physics.particle_spatial_scatter_count = physics_snapshot->particle_spatial_scatter_count;
    buffer->stats.physics.particle_contact_candidate_count = physics_snapshot->particle_contact_candidate_count;
    buffer->stats.physics.particle_body_shape_candidate_count = physics_snapshot->particle_body_shape_candidate_count;
    buffer->stats.physics.particle_barrier_candidate_count = physics_snapshot->particle_barrier_candidate_count;
    buffer->stats.physics.particle_reduction_delta_count = physics_snapshot->particle_reduction_delta_count;
    buffer->stats.physics.particle_reduction_apply_count = physics_snapshot->particle_reduction_apply_count;
    buffer->stats.physics.particle_group_refresh_count = physics_snapshot->particle_group_refresh_count;
    buffer->stats.physics.particle_compaction_move_count = physics_snapshot->particle_compaction_move_count;
    buffer->stats.physics.particle_compaction_remap_count = physics_snapshot->particle_compaction_remap_count;
    buffer->stats.physics.particle_byte_count = physics_snapshot->particle_byte_count;
    buffer->stats.physics.particle_scratch_byte_count = physics_snapshot->particle_scratch_byte_count;
    buffer->stats.physics.particle_scratch_high_water_byte_count =
        physics_snapshot->particle_scratch_high_water_byte_count;
    buffer->stats.physics.particle_profile_ms = physics_snapshot->particle_profile_ms;
    buffer->stats.physics.particle_profile_lifetimes_ms = physics_snapshot->particle_profile_lifetimes_ms;
    buffer->stats.physics.particle_profile_proxies_ms = physics_snapshot->particle_profile_proxies_ms;
    buffer->stats.physics.particle_profile_spatial_index_ms = physics_snapshot->particle_profile_spatial_index_ms;
    buffer->stats.physics.particle_profile_contacts_ms = physics_snapshot->particle_profile_contacts_ms;
    buffer->stats.physics.particle_profile_body_contacts_ms = physics_snapshot->particle_profile_body_contacts_ms;
    buffer->stats.physics.particle_profile_weights_ms = physics_snapshot->particle_profile_weights_ms;
    buffer->stats.physics.particle_profile_forces_ms = physics_snapshot->particle_profile_forces_ms;
    buffer->stats.physics.particle_profile_pressure_ms = physics_snapshot->particle_profile_pressure_ms;
    buffer->stats.physics.particle_profile_damping_ms = physics_snapshot->particle_profile_damping_ms;
    buffer->stats.physics.particle_profile_reduction_ms = physics_snapshot->particle_profile_reduction_ms;
    buffer->stats.physics.particle_profile_collision_ms = physics_snapshot->particle_profile_collision_ms;
    buffer->stats.physics.particle_profile_groups_ms = physics_snapshot->particle_profile_groups_ms;
    buffer->stats.physics.particle_profile_barrier_ms = physics_snapshot->particle_profile_barrier_ms;
    buffer->stats.physics.particle_profile_compaction_ms = physics_snapshot->particle_profile_compaction_ms;
    buffer->stats.physics.particle_profile_scratch_ms = physics_snapshot->particle_profile_scratch_ms;
    buffer->stats.physics.particle_profile_events_ms = physics_snapshot->particle_profile_events_ms;
    buffer->stats.physics.physics_task_worker_count = physics_snapshot->task_worker_count;
    buffer->stats.physics.physics_task_background_thread_count = physics_snapshot->task_background_thread_count;
    buffer->stats.physics.physics_task_enqueued_count = physics_snapshot->corephys_enqueued_task_count;
    buffer->stats.physics.physics_task_inline_count = physics_snapshot->corephys_inline_task_count;
    buffer->stats.physics.physics_task_main_chunk_count = physics_snapshot->corephys_main_chunk_count;
    buffer->stats.physics.physics_task_worker_chunk_count = physics_snapshot->corephys_worker_chunk_count;
    {
        SceneView scene_view = Scene_GetViewSnapshot(desc->scene);
        buffer->stats.camera.camera_x = scene_view.x;
        buffer->stats.camera.camera_y = scene_view.y;
    }

    if (contact_hit_capacity > 0U &&
        buffer->world.draw_debug_world && Scene_GetPhysicsWorld(desc->scene) != NULL &&
        buffer->world.debug_collisions != NULL && buffer->world.debug_collision_capacity > 0U)
    {
        PhysicsContactHit* contact_hits;
        size_t contact_hit_count;
        size_t collision_index = 0U;
        size_t event_index = 0U;

        contact_hits = (PhysicsContactHit*)calloc(contact_hit_capacity, sizeof(*contact_hits));
        if (contact_hits == NULL)
        {
            return false;
        }
        contact_hit_count = PhysicsWorld_GetContactHits(Scene_GetPhysicsWorld(desc->scene), contact_hits, contact_hit_capacity);
        for (event_index = 0U; event_index < contact_hit_count &&
                               collision_index < buffer->world.debug_collision_capacity; ++event_index)
        {
            const PhysicsContactHit* hit = &contact_hits[event_index];
            DebugCollisionView* collision = &buffer->world.debug_collisions[collision_index++];

            collision->point_a = hit->point;
            collision->point_b = hit->point;
            collision->contact_point = hit->point;
            collision->contact_normal = hit->normal;
            collision->active = true;
            collision->sensor = false;
            collision->sleeping = false;
        }
        free(contact_hits);
        buffer->world.debug_collision_count = collision_index;
    }

    return true;
}

bool scene_render_snapshot_build(
    SceneRenderSnapshotBuilder* builder,
    const TaskSystem* task_system,
    struct Scene* scene,
    const Camera* camera,
    Aabb view_bounds,
    float render_alpha,
    uint64_t now_ms,
    double update_ms,
    uint32_t physics_substeps,
    bool debug_overlay_enabled,
    bool draw_debug_world,
    uint64_t selected_entity_id,
    uint64_t hovered_entity_id,
    RendererBackdropDrawFn backdrop_draw,
    void* backdrop_user_data,
    uint64_t backdrop_signature,
    RenderSnapshotBuffer* buffer
) {
    PhysicsWorldSnapshot physics_snapshot;
    SceneRenderSnapshotDesc desc;

    if (builder == NULL || scene == NULL || buffer == NULL) {
        return false;
    }
    if (!scene_render_snapshot_builder_reserve_entities(builder, Scene_GetEntityCount(scene))) {
        return false;
    }

    memset(&physics_snapshot, 0, sizeof(physics_snapshot));
    if (Scene_GetPhysicsWorld(scene) != NULL) {
        PhysicsWorld_GetSnapshot(Scene_GetPhysicsWorld(scene), &physics_snapshot);
    }
    if (!scene_render_snapshot_builder_reserve_particles(builder, physics_snapshot.particle_count)) {
        return false;
    }

    memset(&desc, 0, sizeof(desc));
    desc.scene = scene;
    desc.task_system = task_system;
    desc.builder = builder;
    desc.scratch_entities = builder->scratch_entities;
    desc.scratch_entity_capacity = builder->scratch_entity_capacity;
    desc.scratch_particles = builder->scratch_particles;
    desc.scratch_particle_capacity = builder->scratch_particle_capacity;
    if (camera != NULL)
    {
        desc.camera = *camera;
        desc.has_camera = true;
    }
    desc.view_bounds = view_bounds;
    desc.render_alpha = render_alpha;
    desc.now_ms = now_ms;
    desc.update_ms = update_ms;
    desc.physics_substeps = physics_substeps;
    desc.debug_overlay_enabled = debug_overlay_enabled;
    desc.draw_debug_world = draw_debug_world;
    desc.selected_entity_id = selected_entity_id;
    desc.hovered_entity_id = hovered_entity_id;
    desc.backdrop_draw = backdrop_draw;
    desc.backdrop_user_data = backdrop_user_data;
    desc.backdrop_signature = backdrop_signature;
    desc.physics_snapshot = &physics_snapshot;

    return scene_render_snapshot_build_desc(&desc, buffer);
}
