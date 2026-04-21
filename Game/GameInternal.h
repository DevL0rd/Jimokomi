#ifndef JIMOKOMI_GAME_INTERNAL_H
#define JIMOKOMI_GAME_INTERNAL_H

#include "Game/BallInteractionComponent.h"
#include "Game/BallVisuals.h"
#include "Game/GameInputPacket.h"

#include "../Engine/Engine.h"
#include "../Engine/RuntimeConfig.h"
#include "../Engine/Core/InputPacketStream.h"
#include "../Engine/Core/TaskSystem.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/Scene/Entity.h"
#include "../Engine/Scene/Components/RandomForceComponent.h"
#include "../Engine/Scene/Components/ColliderComponent.h"
#include "../Engine/Scene/Components/RigidBodyComponent.h"
#include "../Engine/Scene/Components/TransformComponent.h"
#include "../Engine/Rendering/RaylibBackend.h"
#include "../Engine/Rendering/Renderer.h"
#include "../Engine/Rendering/RenderSnapshot.h"
#include "../Engine/Rendering/ResourceCommandQueue.h"

#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#define BALL_COUNT 5000
#define SOURCE_VARIANT_COUNT 8
#define INVALID_INDEX ((size_t)-1)
#define PLAYER_INDEX 0U
#define WORLD_WIDTH 1920.0f
#define WORLD_HEIGHT 1080.0f
#define GRID_CELL_SIZE 96.0f
#define PHYSICS_PIXELS_PER_METER 100.0f
#define PHYSICS_EARTH_GRAVITY_MPS2 9.81f
#define PLAYER_MOVE_SPEED 160.0f
#define PLAYER_MOVE_SMOOTHING 0.085f
#define PLAYER_JUMP_IMPULSE 585.0f
#define WORLD_WALL_THICKNESS 32.0f
#define BALL_DIAMETER 28.0f
#define BALL_RADIUS 14.0f
#define BALL_SPAWN_INTERVAL_SECONDS 0.01
#define BALL_SPAWN_WIGGLE_X 24.0f
#define BALL_RANDOM_FORCE_STRENGTH 18.0f
#define BALL_RANDOM_FORCE_INTERVAL_SECONDS 0.08f
#define CAMERA_PAN_KEY_SPEED 640.0f
#define CAMERA_ZOOM_STEP 0.12f
#define CAMERA_ZOOM_KEY_SPEED 5.0f
#define CAMERA_PAN_CLICK_THRESHOLD 4.0f
#define CAMERA_MIN_VIEW_WIDTH 160.0f
#define CAMERA_MIN_VIEW_HEIGHT 90.0f
#define SPATIAL_QUERY_PRELOAD_SCREENS 1.0f
#define SIM_SNAPSHOT_PUBLISH_INTERVAL_MS 16ULL
#define SIM_IDLE_SLEEP_MS 1U

typedef struct Ball {
    Entity* entity;
    TransformComponent* transform;
    RigidBodyComponent* rigid_body;
    ColliderComponent* collider;
    BallInteractionComponent* interaction;
    float radius;
    ResourceHandle visual_source_handle;
    ResourceHandle material_handle;
    uint32_t visual_state_version;
    uint32_t highlight_state_version;
    bool visual_state_dirty;
    bool highlight_state_dirty;
    bool is_selected;
    bool is_hovered;
} Ball;

typedef struct GameState {
    Engine engine;
    RaylibBackend backend;
    Renderer* renderer;
    Scene* scene;
    TaskSystem task_system;
    Ball balls[BALL_COUNT];
    size_t tracked_ball_index;
    size_t selected_ball_index;
    size_t hovered_ball_index;
    uint64_t last_hover_snapshot_sequence;
    int last_hover_mouse_x;
    int last_hover_mouse_y;
    float last_hover_camera_x;
    float last_hover_camera_y;
    float last_hover_camera_view_width;
    float last_hover_camera_view_height;
    bool last_hover_pointer_over_ui;
    bool hover_cache_valid;
    uint32_t hover_pick_query_count;
    uint32_t hover_pick_skip_count;
    uint32_t hover_query_dirty_count;
    uint32_t camera_rect_dirty_count;
    uint32_t camera_rect_stable_count;
    size_t dragged_ball_index;
    bool entity_drag_active;
    Vec2 drag_world_position;
    Vec2 drag_release_velocity;
    bool camera_pan_active;
    int camera_pan_start_mouse_x;
    int camera_pan_start_mouse_y;
    float camera_pan_start_x;
    float camera_pan_start_y;
    double accumulator_seconds;
    double sim_step_dt_seconds;
    double cached_sim_fixed_dt_seconds;
    uint32_t cached_sim_max_substeps;
    Aabb cached_snapshot_view_bounds;
    float cached_snapshot_camera_x;
    float cached_snapshot_camera_y;
    float cached_snapshot_camera_view_width;
    float cached_snapshot_camera_view_height;
    bool cached_snapshot_has_input;
    bool cached_snapshot_view_bounds_valid;
    bool cached_debug_overlay_enabled;
    bool cached_draw_debug_world;
    uint64_t last_tick_ms;
    WorldBackdropConfig backdrop;
    ResourceHandle shared_ball_shader_handle;
    ResourceHandle ball_source_handles[SOURCE_VARIANT_COUNT];
    ResourceHandle ball_material_handles[BALL_COUNT];
    Entity* visible_query_entities[BALL_COUNT];
    size_t active_ball_count;
    size_t spawn_cursor;
    double spawn_accumulator_seconds;
    uint64_t last_snapshot_metadata_signature;
    uint32_t last_spawn_region_dirty_x;
    uint32_t last_spawn_region_dirty_y;
    uint32_t spawn_region_dirty_count;
    uint32_t last_player_neighborhood_count;
} GameState;

typedef struct SimThreadContext {
    GameState* game;
    RenderSnapshotExchange* render_snapshot_exchange;
    InputPacketStream* input_stream;
    ResourceCommandQueue* resource_command_queue;
    atomic_bool shutdown_requested;
} SimThreadContext;

typedef struct GameCullProfile {
    double grid_ms;
    uint32_t grid_candidates;
} GameCullProfile;

float game_lerp(float a, float b, float alpha);
bool game_camera_rect_changed(const GameState* game, const Camera* camera);
float game_random_range(float min_value, float max_value);
uint32_t game_world_cell_x(float x);
uint32_t game_world_cell_y(float y);
uint64_t game_hash_u64(uint64_t hash, uint64_t value);
uint64_t game_backdrop_signature(const WorldBackdropConfig* backdrop);
void game_mark_backdrop_dirty(WorldBackdropConfig* backdrop);
void game_clear_backdrop_dirty(WorldBackdropConfig* backdrop);
void game_mark_ball_visual_state_dirty(Ball* ball);
void game_clear_ball_visual_state_dirty(Ball* ball);
void game_mark_ball_highlight_state(Ball* ball, bool selected, bool hovered);
void game_clear_ball_highlight_dirty(Ball* ball);
void game_sync_ball_highlight_state(GameState* game);
float game_compute_render_alpha(const GameState* game);
float game_lerp_angle(float a, float b, float alpha);
double game_now_ms(void);
void game_sleep_ms(uint32_t milliseconds);

void game_init_world(GameState* game);
bool game_player_is_grounded(const GameState* game);
size_t game_query_player_neighborhood(GameState* game, float radius);
void game_refresh_sim_step_config(GameState* game);
uint32_t game_step_world(GameState* game, double dt_seconds, const EngineInput* input);
void game_update_spawn_curve(GameState* game, double dt_seconds);
void game_apply_entity_drag_packet(GameState* game, const GameInputPacket* input_packet);

Aabb game_compute_snapshot_view_bounds(const GameState* game, const GameInputPacket* input_packet, bool has_input);
Aabb game_get_cached_snapshot_view_bounds(GameState* game, const GameInputPacket* input_packet, bool has_input);
size_t game_collect_frame_data(
    GameState *game,
    SpriteRenderable *items,
    DebugEntityView *entities,
    size_t item_capacity,
    float render_alpha,
    bool capture_all_debug_details,
    size_t selected_ball_index,
    size_t hovered_ball_index,
    Aabb view_bounds,
    GameCullProfile* cull_profile,
    uint64_t* out_item_frame_signature,
    uint64_t* out_item_sort_signature,
    uint64_t* out_item_instance_signature,
    bool* out_items_sorted_by_layer,
    DebugEntityView* out_selected_entity,
    bool* out_has_selected_entity,
    DebugEntityView* out_hovered_entity,
    bool* out_has_hovered_entity,
    PickTargetView* pick_targets,
    size_t pick_target_capacity,
    size_t* out_pick_target_count);
void game_build_render_snapshot(
    GameState* game,
    RenderSnapshotBuffer* buffer,
    float render_alpha,
    uint64_t now_ms,
    double update_ms,
    uint32_t physics_substeps,
    bool debug_overlay_enabled,
    bool draw_debug_world,
    Aabb view_bounds,
    GameCullProfile* cull_profile,
    const PhysicsWorldSnapshot* physics_snapshot);

void game_get_camera_viewport_size(const Renderer* renderer, float* out_width, float* out_height);
void game_render_clamp_camera(Camera* camera);
void game_apply_render_camera_zoom(Renderer* renderer, float zoom_steps, Vec2 anchor_screen);
size_t game_pick_ball_from_snapshot(const RenderSnapshotBuffer* render_snapshot, const Camera* camera, Vec2 screen_point);
void game_update_render_interactions(
    GameState* game,
    Renderer* renderer,
    const EngineInput* input,
    const RenderSnapshotBuffer* render_snapshot,
    bool pointer_over_ui,
    double dt_seconds);
void game_emit_profiler_metadata(
    GameState* game,
    const RenderSnapshotBuffer* render_snapshot,
    const RendererFrame* frame,
    size_t drained_resource_commands,
    uint64_t dropped_render_snapshots,
    const ResourceCommandQueue* resource_command_queue);

void* game_simulation_thread_main(void* user_data);

#endif
