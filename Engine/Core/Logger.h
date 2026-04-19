#ifndef JIMOKOMI_ENGINE_CORE_LOGGER_H
#define JIMOKOMI_ENGINE_CORE_LOGGER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum EngineLogLevel {
    ENGINE_LOG_LEVEL_TRACE = 10,
    ENGINE_LOG_LEVEL_DEBUG = 20,
    ENGINE_LOG_LEVEL_INFO = 30,
    ENGINE_LOG_LEVEL_WARN = 40,
    ENGINE_LOG_LEVEL_ERROR = 50,
    ENGINE_LOG_LEVEL_FATAL = 60
} EngineLogLevel;

typedef struct EngineLoggerConfig {
    const char* path;
    size_t max_lines;
    size_t flush_every;
    bool echo_to_console;
    EngineLogLevel minimum_level;
} EngineLoggerConfig;

typedef struct EngineLogger {
    char* path;
    size_t max_lines;
    size_t flush_every;
    bool echo_to_console;
    EngineLogLevel minimum_level;

    char** lines;
    size_t line_count;
    size_t pending_lines;
} EngineLogger;

bool EngineLogger_init(EngineLogger* logger, const EngineLoggerConfig* config);
void EngineLogger_dispose(EngineLogger* logger);
void EngineLogger_flush(EngineLogger* logger);
bool EngineLogger_should_log(const EngineLogger* logger, EngineLogLevel level);
void EngineLogger_log(EngineLogger* logger, EngineLogLevel level, const char* message, const char* context);
void EngineLogger_trace(EngineLogger* logger, const char* message, const char* context);
void EngineLogger_debug(EngineLogger* logger, const char* message, const char* context);
void EngineLogger_info(EngineLogger* logger, const char* message, const char* context);
void EngineLogger_warn(EngineLogger* logger, const char* message, const char* context);
void EngineLogger_error(EngineLogger* logger, const char* message, const char* context);
void EngineLogger_fatal(EngineLogger* logger, const char* message, const char* context);
size_t EngineLogger_copy_recent_lines(const EngineLogger* logger, size_t count, const char** output_lines, size_t output_capacity);

#endif
