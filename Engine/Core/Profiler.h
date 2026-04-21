#ifndef JIMOKOMI_ENGINE_CORE_PROFILER_H
#define JIMOKOMI_ENGINE_CORE_PROFILER_H

#include <stdbool.h>
#include <stddef.h>

#define ENGINE_PROFILER_MAX_SCOPES_PER_FRAME 128
#define ENGINE_PROFILER_MAX_SCOPE_DEPTH 32
#define ENGINE_PROFILER_MAX_METADATA_ITEMS 192
#define ENGINE_PROFILER_SCOPE_LOOKUP_CAPACITY 257
#define ENGINE_PROFILER_METADATA_LOOKUP_CAPACITY 389
#define ENGINE_PROFILER_NAME_CAPACITY 64
#define ENGINE_PROFILER_VALUE_CAPACITY 128

typedef struct EngineProfilerConfig {
    bool enabled;
    const char* path;
    const char* text_path;
    size_t max_frames;
    size_t flush_every;
} EngineProfilerConfig;

typedef struct EngineProfilerScopeSample {
    char name[ENGINE_PROFILER_NAME_CAPACITY];
    double total_ms;
    size_t calls;
} EngineProfilerScopeSample;

typedef struct EngineProfilerMetadataItem {
    char key[ENGINE_PROFILER_NAME_CAPACITY];
    char value[ENGINE_PROFILER_VALUE_CAPACITY];
} EngineProfilerMetadataItem;

typedef struct EngineProfilerFrame {
    char name[ENGINE_PROFILER_NAME_CAPACITY];
    double started_ms;
    double ended_ms;
    double total_ms;

    EngineProfilerScopeSample scopes[ENGINE_PROFILER_MAX_SCOPES_PER_FRAME];
    size_t scope_count;

    EngineProfilerMetadataItem metadata[ENGINE_PROFILER_MAX_METADATA_ITEMS];
    size_t metadata_count;
} EngineProfilerFrame;

typedef struct EngineProfilerSnapshot {
    size_t frame_count;
    const EngineProfilerFrame* last_frame;
} EngineProfilerSnapshot;

typedef struct EngineProfiler {
    bool enabled;
    char* path;
    char* text_path;
    size_t max_frames;
    size_t flush_every;
    size_t flush_counter;

    EngineProfilerFrame* frame_history;
    size_t frame_count;
    size_t frame_capacity;

    EngineProfilerFrame current_frame;
    bool has_current_frame;

    struct {
        char name[ENGINE_PROFILER_NAME_CAPACITY];
        double started_ms;
    } scope_stack[ENGINE_PROFILER_MAX_SCOPE_DEPTH];
    size_t scope_stack_count;
    int scope_lookup[ENGINE_PROFILER_SCOPE_LOOKUP_CAPACITY];
    int metadata_lookup[ENGINE_PROFILER_METADATA_LOOKUP_CAPACITY];
} EngineProfiler;

bool EngineProfiler_init(EngineProfiler* profiler, const EngineProfilerConfig* config);
void EngineProfiler_dispose(EngineProfiler* profiler);
void EngineProfiler_begin_frame(EngineProfiler* profiler, const char* frame_name);
void EngineProfiler_begin_frame_with_metadata(EngineProfiler* profiler, const char* frame_name, const char* metadata_key, const char* metadata_value);
void EngineProfiler_begin_scope(EngineProfiler* profiler, const char* name);
void EngineProfiler_end_scope(EngineProfiler* profiler, const char* name);
void EngineProfiler_set_metadata_string(EngineProfiler* profiler, const char* key, const char* value);
void EngineProfiler_set_metadata_number(EngineProfiler* profiler, const char* key, double value);
void EngineProfiler_set_metadata_bool(EngineProfiler* profiler, const char* key, bool value);
void EngineProfiler_end_frame(EngineProfiler* profiler);
void EngineProfiler_flush(EngineProfiler* profiler);
EngineProfilerSnapshot EngineProfiler_get_snapshot(EngineProfiler* profiler);
const EngineProfilerFrame* EngineProfiler_get_last_frame_by_name(const EngineProfiler* profiler, const char* frame_name);
double EngineProfiler_get_average_scope_for_frame(const EngineProfiler* profiler, const char* frame_name, const char* scope_name);

#endif
