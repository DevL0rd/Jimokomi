#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_H

#include "Camera.h"
#include "DebugOverlay.h"
#include "ResourceManager.h"
#include "Target.h"

struct Scene;

typedef struct SpriteRenderable {
    float x;
    float y;
    float angle_radians;
    float anchor_x;
    float anchor_y;
    int layer;
    bool visible;
    ResourceHandle visual_source_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    void* user_data;
} SpriteRenderable;

typedef struct RendererConfig {
    int view_width;
    int view_height;
    size_t prebake_budget_per_frame;
    size_t prebake_admission_total_hits;
    size_t prebake_admission_frame_hits;
} RendererConfig;

typedef void (*RendererBackdropDrawFn)(Target* target, const Camera* camera, void* user_data);

typedef struct RendererFrame {
    const SpriteRenderable *items;
    size_t item_count;
    RendererBackdropDrawFn backdrop_draw;
    void* backdrop_user_data;
    DebugEntityView *debug_entities;
    size_t debug_entity_count;
    DebugCollisionView *debug_collisions;
    size_t debug_collision_count;
    bool draw_sprites;
    bool draw_debug;
    uint64_t now_ms;
} RendererFrame;

typedef struct Renderer {
    DebugOverlay debug_overlay;
    Camera camera;
    ResourceManager resource_manager;
    int view_width;
    int view_height;
    SpriteRenderable* scratch_items;
    size_t scratch_capacity;
    SurfaceDrawInstance* scratch_instances;
    size_t scratch_instance_capacity;
    size_t last_render_item_count;
    size_t last_sprite_draw_count;
    size_t last_visible_item_count;
    size_t last_overlay_draw_count;
    size_t last_procedural_item_count;
    size_t last_baked_surface_count;
    size_t last_instanced_batch_count;
    size_t last_instanced_draw_count;
    double last_sort_ms;
    double last_visibility_ms;
    double last_body_draw_ms;
    double last_overlay_draw_ms;
    double last_instance_draw_ms;
} Renderer;

void renderer_init(Renderer *renderer, RenderBackend *backend, const RendererConfig *config);
void renderer_dispose(Renderer *renderer);
void renderer_draw(Renderer *renderer, RenderBackend *backend, const RendererFrame *frame);

#endif
