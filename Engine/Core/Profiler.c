#include "Profiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char* engine_profiler_strdup(const char* source) {
    char* copy = 0;
    size_t length = 0;

    if (source == 0) {
        return 0;
    }

    length = strlen(source);
    copy = (char*)malloc(length + 1);
    if (copy == 0) {
        return 0;
    }

    memcpy(copy, source, length + 1);
    return copy;
}

static double engine_profiler_now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static void engine_profiler_copy_string(char* destination, size_t destination_size, const char* source) {
    if (destination == 0 || destination_size == 0) {
        return;
    }

    if (source == 0) {
        destination[0] = '\0';
        return;
    }

    snprintf(destination, destination_size, "%s", source);
}

static EngineProfilerScopeSample* engine_profiler_find_scope(EngineProfilerFrame* frame, const char* name) {
    size_t index = 0;

    if (frame == 0 || name == 0) {
        return 0;
    }

    for (index = 0; index < frame->scope_count; ++index) {
        if (strncmp(frame->scopes[index].name, name, ENGINE_PROFILER_NAME_CAPACITY) == 0) {
            return &frame->scopes[index];
        }
    }

    if (frame->scope_count >= ENGINE_PROFILER_MAX_SCOPES_PER_FRAME) {
        return 0;
    }

    engine_profiler_copy_string(frame->scopes[frame->scope_count].name, ENGINE_PROFILER_NAME_CAPACITY, name);
    frame->scopes[frame->scope_count].total_ms = 0.0;
    frame->scopes[frame->scope_count].calls = 0;
    frame->scope_count += 1;
    return &frame->scopes[frame->scope_count - 1];
}

static EngineProfilerMetadataItem* engine_profiler_find_metadata(EngineProfilerFrame* frame, const char* key) {
    size_t index = 0;

    if (frame == 0 || key == 0) {
        return 0;
    }

    for (index = 0; index < frame->metadata_count; ++index) {
        if (strncmp(frame->metadata[index].key, key, ENGINE_PROFILER_NAME_CAPACITY) == 0) {
            return &frame->metadata[index];
        }
    }

    if (frame->metadata_count >= ENGINE_PROFILER_MAX_METADATA_ITEMS) {
        return 0;
    }

    engine_profiler_copy_string(frame->metadata[frame->metadata_count].key, ENGINE_PROFILER_NAME_CAPACITY, key);
    frame->metadata[frame->metadata_count].value[0] = '\0';
    frame->metadata_count += 1;
    return &frame->metadata[frame->metadata_count - 1];
}

static void engine_profiler_reset_current_frame(EngineProfiler* profiler) {
    memset(&profiler->current_frame, 0, sizeof(profiler->current_frame));
    profiler->scope_stack_count = 0;
    profiler->has_current_frame = false;
}

static int engine_profiler_compare_scope_desc(const void* left, const void* right) {
    const EngineProfilerScopeSample* a = (const EngineProfilerScopeSample*)left;
    const EngineProfilerScopeSample* b = (const EngineProfilerScopeSample*)right;

    if (a->total_ms < b->total_ms) {
        return 1;
    }
    if (a->total_ms > b->total_ms) {
        return -1;
    }
    return strncmp(a->name, b->name, ENGINE_PROFILER_NAME_CAPACITY);
}

static void engine_profiler_write_text_report(const EngineProfiler* profiler) {
    FILE* file = 0;
    size_t index = 0;
    EngineProfilerScopeSample sorted[ENGINE_PROFILER_MAX_SCOPES_PER_FRAME];

    if (profiler == 0 || profiler->text_path == 0) {
        return;
    }

    file = fopen(profiler->text_path, "w");
    if (file == 0) {
        return;
    }

    fprintf(file, "Performance Profile\n");
    fprintf(file, "frames: %zu\n", profiler->frame_count);

    for (index = 0; index < profiler->frame_count; ++index) {
        const EngineProfilerFrame* frame = &profiler->frame_history[index];
        size_t scope_index = 0;
        size_t metadata_index = 0;

        fprintf(file, "\nFrame %zu\n", index + 1);
        fprintf(file, "name: %s\n", frame->name);
        fprintf(file, "total_ms: %.3f\n", frame->total_ms);
        fprintf(file, "metadata:\n");
        for (metadata_index = 0; metadata_index < frame->metadata_count; ++metadata_index) {
            fprintf(file, "  %s: %s\n", frame->metadata[metadata_index].key, frame->metadata[metadata_index].value);
        }

        memcpy(sorted, frame->scopes, sizeof(EngineProfilerScopeSample) * frame->scope_count);
        qsort(sorted, frame->scope_count, sizeof(EngineProfilerScopeSample), engine_profiler_compare_scope_desc);

        fprintf(file, "scopes:\n");
        for (scope_index = 0; scope_index < frame->scope_count; ++scope_index) {
            fprintf(file, "  %s: %.3fms (%zu calls)\n", sorted[scope_index].name, sorted[scope_index].total_ms, sorted[scope_index].calls);
        }
    }

    fclose(file);
}

bool EngineProfiler_init(EngineProfiler* profiler, const EngineProfilerConfig* config) {
    size_t frame_capacity = 180;

    if (profiler == 0) {
        return false;
    }

    memset(profiler, 0, sizeof(*profiler));
    frame_capacity = (config != 0 && config->max_frames > 0) ? config->max_frames : 180;
    profiler->path = engine_profiler_strdup((config != 0 && config->path != 0) ? config->path : "logs/performance_profile.bin");
    profiler->text_path = engine_profiler_strdup((config != 0 && config->text_path != 0) ? config->text_path : "logs/performance_profile.txt");
    profiler->flush_every = (config != 0 && config->flush_every > 0) ? config->flush_every : 15;
    profiler->max_frames = frame_capacity;
    profiler->frame_capacity = frame_capacity;
    profiler->frame_history = (EngineProfilerFrame*)calloc(frame_capacity, sizeof(EngineProfilerFrame));
    profiler->flush_counter = 0;
    engine_profiler_reset_current_frame(profiler);

    return profiler->path != 0 && profiler->text_path != 0 && profiler->frame_history != 0;
}

void EngineProfiler_dispose(EngineProfiler* profiler) {
    if (profiler == 0) {
        return;
    }

    EngineProfiler_flush(profiler);
    free(profiler->path);
    free(profiler->text_path);
    free(profiler->frame_history);
    profiler->path = 0;
    profiler->text_path = 0;
    profiler->frame_history = 0;
    profiler->frame_capacity = 0;
    profiler->frame_count = 0;
    profiler->has_current_frame = false;
}

void EngineProfiler_begin_frame(EngineProfiler* profiler, const char* frame_name) {
    if (profiler == 0) {
        return;
    }

    memset(&profiler->current_frame, 0, sizeof(profiler->current_frame));
    profiler->scope_stack_count = 0;
    profiler->has_current_frame = true;
    engine_profiler_copy_string(profiler->current_frame.name, ENGINE_PROFILER_NAME_CAPACITY, frame_name != 0 ? frame_name : "frame");
    profiler->current_frame.started_ms = engine_profiler_now_ms();
}

void EngineProfiler_begin_frame_with_metadata(EngineProfiler* profiler, const char* frame_name, const char* metadata_key, const char* metadata_value) {
    EngineProfiler_begin_frame(profiler, frame_name);
    EngineProfiler_set_metadata_string(profiler, metadata_key, metadata_value);
}

void EngineProfiler_begin_scope(EngineProfiler* profiler, const char* name) {
    if (profiler == 0 || !profiler->has_current_frame || name == 0 || profiler->scope_stack_count >= ENGINE_PROFILER_MAX_SCOPE_DEPTH) {
        return;
    }

    engine_profiler_copy_string(profiler->scope_stack[profiler->scope_stack_count].name, ENGINE_PROFILER_NAME_CAPACITY, name);
    profiler->scope_stack[profiler->scope_stack_count].started_ms = engine_profiler_now_ms();
    profiler->scope_stack_count += 1;
}

void EngineProfiler_end_scope(EngineProfiler* profiler, const char* name) {
    double duration_ms = 0.0;
    const char* scope_name = 0;
    EngineProfilerScopeSample* scope = 0;

    if (profiler == 0 || !profiler->has_current_frame || profiler->scope_stack_count == 0) {
        return;
    }

    profiler->scope_stack_count -= 1;
    scope_name = name != 0 ? name : profiler->scope_stack[profiler->scope_stack_count].name;
    duration_ms = engine_profiler_now_ms() - profiler->scope_stack[profiler->scope_stack_count].started_ms;
    if (duration_ms < 0.0) {
        duration_ms = 0.0;
    }

    scope = engine_profiler_find_scope(&profiler->current_frame, scope_name);
    if (scope == 0) {
        return;
    }

    scope->total_ms += duration_ms;
    scope->calls += 1;
}

void EngineProfiler_set_metadata_string(EngineProfiler* profiler, const char* key, const char* value) {
    EngineProfilerMetadataItem* item = 0;

    if (profiler == 0 || !profiler->has_current_frame) {
        return;
    }

    item = engine_profiler_find_metadata(&profiler->current_frame, key);
    if (item == 0) {
        return;
    }

    engine_profiler_copy_string(item->value, ENGINE_PROFILER_VALUE_CAPACITY, value != 0 ? value : "");
}

void EngineProfiler_set_metadata_number(EngineProfiler* profiler, const char* key, double value) {
    char buffer[ENGINE_PROFILER_VALUE_CAPACITY];

    snprintf(buffer, sizeof(buffer), "%.3f", value);
    EngineProfiler_set_metadata_string(profiler, key, buffer);
}

void EngineProfiler_set_metadata_bool(EngineProfiler* profiler, const char* key, bool value) {
    EngineProfiler_set_metadata_string(profiler, key, value ? "true" : "false");
}

void EngineProfiler_end_frame(EngineProfiler* profiler) {
    if (profiler == 0 || !profiler->has_current_frame) {
        return;
    }

    while (profiler->scope_stack_count > 0) {
        EngineProfiler_end_scope(profiler, 0);
    }

    profiler->current_frame.ended_ms = engine_profiler_now_ms();
    profiler->current_frame.total_ms = profiler->current_frame.ended_ms - profiler->current_frame.started_ms;
    if (profiler->current_frame.total_ms < 0.0) {
        profiler->current_frame.total_ms = 0.0;
    }

    if (profiler->frame_count < profiler->frame_capacity) {
        profiler->frame_history[profiler->frame_count++] = profiler->current_frame;
    } else if (profiler->frame_capacity > 0) {
        memmove(profiler->frame_history, profiler->frame_history + 1, sizeof(EngineProfilerFrame) * (profiler->frame_capacity - 1));
        profiler->frame_history[profiler->frame_capacity - 1] = profiler->current_frame;
    }

    profiler->flush_counter += 1;
    profiler->has_current_frame = false;

    if (profiler->flush_counter >= profiler->flush_every) {
        EngineProfiler_flush(profiler);
    }
}

void EngineProfiler_flush(EngineProfiler* profiler) {
    FILE* file = 0;

    if (profiler == 0 || profiler->path == 0) {
        return;
    }

    profiler->flush_counter = 0;
    file = fopen(profiler->path, "wb");
    if (file != 0) {
        fwrite(&profiler->frame_count, sizeof(profiler->frame_count), 1, file);
        fwrite(profiler->frame_history, sizeof(EngineProfilerFrame), profiler->frame_count, file);
        fclose(file);
    }

    engine_profiler_write_text_report(profiler);
}

EngineProfilerSnapshot EngineProfiler_get_snapshot(EngineProfiler* profiler) {
    EngineProfilerSnapshot snapshot;

    snapshot.frame_count = profiler != 0 ? profiler->frame_count : 0;
    snapshot.last_frame = (profiler != 0 && profiler->frame_count > 0) ? &profiler->frame_history[profiler->frame_count - 1] : 0;
    return snapshot;
}

const EngineProfilerFrame* EngineProfiler_get_last_frame_by_name(const EngineProfiler* profiler, const char* frame_name) {
    size_t index = 0;

    if (profiler == 0 || frame_name == 0) {
        return 0;
    }

    for (index = profiler->frame_count; index > 0; --index) {
        const EngineProfilerFrame* frame = &profiler->frame_history[index - 1];
        if (strncmp(frame->name, frame_name, ENGINE_PROFILER_NAME_CAPACITY) == 0) {
            return frame;
        }
    }

    return 0;
}

double EngineProfiler_get_average_scope_for_frame(const EngineProfiler* profiler, const char* frame_name, const char* scope_name) {
    size_t frame_index = 0;
    double total = 0.0;
    size_t count = 0;

    if (profiler == 0 || frame_name == 0 || scope_name == 0) {
        return 0.0;
    }

    for (frame_index = 0; frame_index < profiler->frame_count; ++frame_index) {
        const EngineProfilerFrame* frame = &profiler->frame_history[frame_index];
        size_t scope_index = 0;

        if (strncmp(frame->name, frame_name, ENGINE_PROFILER_NAME_CAPACITY) != 0) {
            continue;
        }

        for (scope_index = 0; scope_index < frame->scope_count; ++scope_index) {
            if (strncmp(frame->scopes[scope_index].name, scope_name, ENGINE_PROFILER_NAME_CAPACITY) == 0) {
                total += frame->scopes[scope_index].total_ms;
                count += 1;
                break;
            }
        }
    }

    return count > 0 ? total / (double)count : 0.0;
}
