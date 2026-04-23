#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include "PathResolver.h"

#include "Memory.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#if defined(_WIN32)
#define ENGINE_PATH_SEPARATOR '\\'
#else
#define ENGINE_PATH_SEPARATOR '/'
#endif

#if defined(_WIN32) && !defined(PATH_MAX)
#define PATH_MAX MAX_PATH
#endif

static char* engine_pathresolver_strdup(const char* source) {
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

static int engine_pathresolver_is_separator(char value) {
    return value == '/' || value == '\\';
}

static int engine_pathresolver_is_absolute(const char* path) {
    if (path == 0 || path[0] == '\0') {
        return 0;
    }

#if defined(_WIN32)
    if (engine_pathresolver_is_separator(path[0]) && engine_pathresolver_is_separator(path[1])) {
        return 1;
    }
    if (isalpha((unsigned char)path[0]) && path[1] == ':' && engine_pathresolver_is_separator(path[2])) {
        return 1;
    }
#endif

    return engine_pathresolver_is_separator(path[0]);
}

static char* engine_pathresolver_get_executable_directory(void) {
    char buffer[PATH_MAX];
    char* slash = 0;
    char* directory = 0;

    memset(buffer, 0, sizeof(buffer));

#if defined(_WIN32)
    {
        DWORD length = GetModuleFileNameA(NULL, buffer, (DWORD)sizeof(buffer));
        if (length == 0 || length >= sizeof(buffer)) {
            return 0;
        }
    }
#else
    {
        ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (length <= 0) {
            return 0;
        }

        buffer[length] = '\0';
    }
#endif

    slash = strrchr(buffer, '/');
#if defined(_WIN32)
    {
        char* backslash = strrchr(buffer, '\\');
        if (backslash != 0 && (slash == 0 || backslash > slash)) {
            slash = backslash;
        }
    }
#endif
    if (slash == 0) {
        return 0;
    }

    *slash = '\0';
    directory = engine_pathresolver_strdup(buffer);
    return directory;
}

char* EnginePathResolver_resolve_relative_to_executable(const char* path) {
    char* executable_directory = 0;
    char* resolved = 0;
    size_t directory_length = 0;
    size_t path_length = 0;

    if (path == 0) {
        return 0;
    }

    if (engine_pathresolver_is_absolute(path)) {
        return engine_pathresolver_strdup(path);
    }

    executable_directory = engine_pathresolver_get_executable_directory();
    if (executable_directory == 0) {
        return 0;
    }

    directory_length = strlen(executable_directory);
    path_length = strlen(path);
    resolved = (char*)malloc(directory_length + 1 + path_length + 1);
    if (resolved != 0) {
        memcpy(resolved, executable_directory, directory_length);
        resolved[directory_length] = ENGINE_PATH_SEPARATOR;
        memcpy(resolved + directory_length + 1, path, path_length + 1);
    }

    free(executable_directory);
    return resolved;
}

static int engine_pathresolver_mkdir(const char* path) {
#if defined(_WIN32)
    return _mkdir(path);
#else
    return mkdir(path, 0777);
#endif
}

static size_t engine_pathresolver_parent_scan_start(const char* path) {
    size_t index;
    int separators_found;

    if (path == 0 || path[0] == '\0') {
        return 0;
    }

#if defined(_WIN32)
    if (isalpha((unsigned char)path[0]) && path[1] == ':') {
        return engine_pathresolver_is_separator(path[2]) ? 3U : 2U;
    }
    if (engine_pathresolver_is_separator(path[0]) && engine_pathresolver_is_separator(path[1])) {
        separators_found = 0;
        for (index = 2U; path[index] != '\0'; ++index) {
            if (!engine_pathresolver_is_separator(path[index]) ||
                (index > 0U && engine_pathresolver_is_separator(path[index - 1U]))) {
                continue;
            }
            ++separators_found;
            if (separators_found == 2) {
                return index + 1U;
            }
        }
        return index;
    }
#endif

    return engine_pathresolver_is_separator(path[0]) ? 1U : 0U;
}

void EnginePathResolver_ensure_parent_dirs(const char* path) {
    char* buffer = 0;
    size_t index = 0;
    size_t length = 0;
    size_t start = 0;

    if (path == 0) {
        return;
    }

    length = strlen(path);
    if (length == 0) {
        return;
    }

    buffer = engine_pathresolver_strdup(path);
    if (buffer == 0) {
        return;
    }

    start = engine_pathresolver_parent_scan_start(buffer);
    for (index = start; index < length; ++index) {
        if (!engine_pathresolver_is_separator(buffer[index])) {
            continue;
        }

        buffer[index] = '\0';
        if (buffer[0] != '\0' && engine_pathresolver_mkdir(buffer) != 0 && errno != EEXIST) {
            buffer[index] = path[index];
            break;
        }
        buffer[index] = path[index];
    }

    free(buffer);
}
