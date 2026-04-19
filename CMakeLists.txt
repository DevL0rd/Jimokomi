cmake_minimum_required(VERSION 3.22)

project(Jimokomi VERSION 0.1.0 LANGUAGES C)

find_package(Threads REQUIRED)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(BOX2D_DOCS OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_VALIDATE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
set(OPENGL_VERSION "2.1" CACHE STRING "" FORCE)

add_subdirectory(third_party/box2d)
add_subdirectory(third_party/raylib)

set(SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/main.c"
)
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/*.c"
)
file(GLOB_RECURSE ENGINE_C_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/*.c"
)
list(APPEND SOURCES
	${GAME_SOURCES}
	${ENGINE_C_SOURCES}
)

add_executable(jimokomi ${SOURCES})

target_include_directories(jimokomi PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/third_party/box2d/include"
)

target_link_libraries(jimokomi PRIVATE box2d raylib Threads::Threads)

if(UNIX AND NOT APPLE)
	target_compile_definitions(jimokomi PRIVATE _POSIX_C_SOURCE=199309L)
	target_link_libraries(jimokomi PRIVATE m)
endif()

if(MSVC)
	target_compile_definitions(jimokomi PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
	target_compile_options(jimokomi PRIVATE -Wall -Wextra -Wpedantic)
endif()
