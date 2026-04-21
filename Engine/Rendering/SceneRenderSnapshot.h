#ifndef JIMOKOMI_ENGINE_RENDERING_SCENERENDERSNAPSHOT_H
#define JIMOKOMI_ENGINE_RENDERING_SCENERENDERSNAPSHOT_H

#include "RenderSnapshot.h"
#include "../Physics/PhysicsWorld.h"

struct Entity;
struct Scene;

typedef struct SceneRenderSnapshotDesc
{
    struct Scene* scene;
    struct Entity** scratch_entities;
    size_t scratch_entity_capacity;
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

Aabb scene_render_snapshot_compute_view_bounds(
    const struct Scene* scene,
    const Camera* camera,
    float preload_screens
);
void scene_render_snapshot_build(
    const SceneRenderSnapshotDesc* desc,
    RenderSnapshotBuffer* buffer
);

#endif
