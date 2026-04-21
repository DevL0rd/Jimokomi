#ifndef JIMOKOMI_ENGINE_RENDERING_SCENERENDERSNAPSHOT_H
#define JIMOKOMI_ENGINE_RENDERING_SCENERENDERSNAPSHOT_H

#include "RenderSnapshot.h"

struct Entity;
struct Scene;

typedef struct SceneRenderSnapshotBuilder SceneRenderSnapshotBuilder;

SceneRenderSnapshotBuilder* scene_render_snapshot_builder_create(void);
void scene_render_snapshot_builder_destroy(SceneRenderSnapshotBuilder* builder);

Aabb scene_render_snapshot_compute_view_bounds(
    const struct Scene* scene,
    const Camera* camera,
    float preload_screens
);
bool scene_render_snapshot_build(
    SceneRenderSnapshotBuilder* builder,
    struct Scene* scene,
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
);

#endif
