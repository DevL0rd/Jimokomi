#define _POSIX_C_SOURCE 200809L

#include "PathResolver.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

static int engine_pathresolver_is_absolute(const char* path) {
    return path != 0 && path[0] == '/';
}

static char* engine_pathresolver_get_executable_directory(void) {
    char buffer[PATH_MAX];
    ssize_t length = 0;
    char* slash = 0;
    char* directory = 0;

    memset(buffer, 0, sizeof(buffer));
    length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length <= 0) {
        return 0;
    }

    buffer[length] = '\0';
    slash = strrchr(buffer, '/');
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
        resolved[directory_length] = '/';
        memcpy(resolved + directory_length + 1, path, path_length + 1);
    }

    free(executable_directory);
    return resolved;
}

void EnginePathResolver_ensure_parent_dirs(const char* path) {
    char* buffer = 0;
    size_t index = 0;
    size_t length = 0;

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

    for (index = 1; index < length; ++index) {
        if (buffer[index] != '/') {
            continue;
        }

        buffer[index] = '\0';
        if (buffer[0] != '\0' && mkdir(buffer, 0777) != 0 && errno != EEXIST) {
            buffer[index] = '/';
            break;
        }
        buffer[index] = '/';
    }

    free(buffer);
}
