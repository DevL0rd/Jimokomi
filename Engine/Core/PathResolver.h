#ifndef JIMOKOMI_ENGINE_CORE_PATH_RESOLVER_H
#define JIMOKOMI_ENGINE_CORE_PATH_RESOLVER_H

char* EnginePathResolver_resolve_relative_to_executable(const char* path);
void EnginePathResolver_ensure_parent_dirs(const char* path);

#endif
