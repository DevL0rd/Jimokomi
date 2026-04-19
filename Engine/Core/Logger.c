#include "Logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char* engine_logger_strdup(const char* source) {
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

static void engine_logger_clear_lines(EngineLogger* logger) {
    size_t index = 0;

    if (logger == 0 || logger->lines == 0) {
        return;
    }

    for (index = 0; index < logger->line_count; ++index) {
        free(logger->lines[index]);
        logger->lines[index] = 0;
    }

    free(logger->lines);
    logger->lines = 0;
    logger->line_count = 0;
}

static const char* engine_logger_level_name(EngineLogLevel level) {
    switch (level) {
        case ENGINE_LOG_LEVEL_TRACE: return "TRACE";
        case ENGINE_LOG_LEVEL_DEBUG: return "DEBUG";
        case ENGINE_LOG_LEVEL_INFO: return "INFO";
        case ENGINE_LOG_LEVEL_WARN: return "WARN";
        case ENGINE_LOG_LEVEL_ERROR: return "ERROR";
        case ENGINE_LOG_LEVEL_FATAL: return "FATAL";
        default: return "INFO";
    }
}

static void engine_logger_make_timestamp(char* output, size_t output_size) {
    time_t now = 0;
    struct tm tm_value;

    if (output == 0 || output_size == 0) {
        return;
    }

    now = time(0);
    memset(&tm_value, 0, sizeof(tm_value));
#if defined(_POSIX_VERSION)
    localtime_r(&now, &tm_value);
#else
    {
        struct tm* local_tm = localtime(&now);
        if (local_tm != 0) {
            tm_value = *local_tm;
        }
    }
#endif
    strftime(output, output_size, "%Y-%m-%d %H:%M:%S", &tm_value);
}

static void engine_logger_append_line(EngineLogger* logger, const char* line) {
    char** next_lines = 0;
    char* line_copy = 0;

    if (logger == 0 || line == 0) {
        return;
    }

    line_copy = engine_logger_strdup(line);
    if (line_copy == 0) {
        return;
    }

    if (logger->max_lines == 0) {
        free(line_copy);
        return;
    }

    if (logger->line_count >= logger->max_lines) {
        free(logger->lines[0]);
        memmove(logger->lines, logger->lines + 1, sizeof(char*) * (logger->line_count - 1));
        logger->line_count -= 1;
    }

    next_lines = (char**)realloc(logger->lines, sizeof(char*) * (logger->line_count + 1));
    if (next_lines == 0) {
        free(line_copy);
        return;
    }

    logger->lines = next_lines;
    logger->lines[logger->line_count] = line_copy;
    logger->line_count += 1;
    logger->pending_lines += 1;
}

bool EngineLogger_init(EngineLogger* logger, const EngineLoggerConfig* config) {
    EngineLoggerConfig defaults;

    if (logger == 0) {
        return false;
    }

    defaults.path = "logs/runtime.log";
    defaults.max_lines = 800;
    defaults.flush_every = 1;
    defaults.echo_to_console = true;
    defaults.minimum_level = ENGINE_LOG_LEVEL_TRACE;

    memset(logger, 0, sizeof(*logger));
    logger->max_lines = config != 0 && config->max_lines > 0 ? config->max_lines : defaults.max_lines;
    logger->flush_every = config != 0 && config->flush_every > 0 ? config->flush_every : defaults.flush_every;
    logger->echo_to_console = config != 0 ? config->echo_to_console : defaults.echo_to_console;
    logger->minimum_level = config != 0 ? config->minimum_level : defaults.minimum_level;
    logger->path = engine_logger_strdup((config != 0 && config->path != 0) ? config->path : defaults.path);

    return logger->path != 0;
}

void EngineLogger_dispose(EngineLogger* logger) {
    if (logger == 0) {
        return;
    }

    EngineLogger_flush(logger);
    engine_logger_clear_lines(logger);
    free(logger->path);
    logger->path = 0;
    logger->pending_lines = 0;
}

void EngineLogger_flush(EngineLogger* logger) {
    FILE* file = 0;
    size_t index = 0;

    if (logger == 0 || logger->path == 0) {
        return;
    }

    file = fopen(logger->path, "w");
    if (file == 0) {
        return;
    }

    for (index = 0; index < logger->line_count; ++index) {
        fputs(logger->lines[index], file);
        fputc('\n', file);
    }

    fclose(file);
    logger->pending_lines = 0;
}

bool EngineLogger_should_log(const EngineLogger* logger, EngineLogLevel level) {
    if (logger == 0) {
        return false;
    }
    return level >= logger->minimum_level;
}

void EngineLogger_log(EngineLogger* logger, EngineLogLevel level, const char* message, const char* context) {
    char timestamp[32];
    char line[2048];
    int written = 0;

    if (logger == 0 || message == 0 || !EngineLogger_should_log(logger, level)) {
        return;
    }

    engine_logger_make_timestamp(timestamp, sizeof(timestamp));
    written = snprintf(
        line,
        sizeof(line),
        "[%s] [%s] %s%s%s",
        timestamp,
        engine_logger_level_name(level),
        message,
        context != 0 ? " " : "",
        context != 0 ? context : ""
    );

    if (written < 0) {
        return;
    }

    if (logger->echo_to_console) {
        fputs(line, stdout);
        fputc('\n', stdout);
    }

    engine_logger_append_line(logger, line);
    if (logger->pending_lines >= logger->flush_every || level >= ENGINE_LOG_LEVEL_FATAL) {
        EngineLogger_flush(logger);
    }
}

void EngineLogger_trace(EngineLogger* logger, const char* message, const char* context) {
    EngineLogger_log(logger, ENGINE_LOG_LEVEL_TRACE, message, context);
}

void EngineLogger_debug(EngineLogger* logger, const char* message, const char* context) {
    EngineLogger_log(logger, ENGINE_LOG_LEVEL_DEBUG, message, context);
}

void EngineLogger_info(EngineLogger* logger, const char* message, const char* context) {
    EngineLogger_log(logger, ENGINE_LOG_LEVEL_INFO, message, context);
}

void EngineLogger_warn(EngineLogger* logger, const char* message, const char* context) {
    EngineLogger_log(logger, ENGINE_LOG_LEVEL_WARN, message, context);
}

void EngineLogger_error(EngineLogger* logger, const char* message, const char* context) {
    EngineLogger_log(logger, ENGINE_LOG_LEVEL_ERROR, message, context);
}

void EngineLogger_fatal(EngineLogger* logger, const char* message, const char* context) {
    EngineLogger_log(logger, ENGINE_LOG_LEVEL_FATAL, message, context);
}

size_t EngineLogger_copy_recent_lines(const EngineLogger* logger, size_t count, const char** output_lines, size_t output_capacity) {
    size_t start = 0;
    size_t index = 0;
    size_t copied = 0;

    if (logger == 0 || output_lines == 0 || output_capacity == 0) {
        return 0;
    }

    if (count > logger->line_count) {
        count = logger->line_count;
    }

    start = logger->line_count > count ? logger->line_count - count : 0;
    for (index = start; index < logger->line_count && copied < output_capacity; ++index) {
        output_lines[copied++] = logger->lines[index];
    }

    return copied;
}
