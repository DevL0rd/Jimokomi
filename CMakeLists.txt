cmake_minimum_required(VERSION 3.22)

project(Jimokomi VERSION 0.1.0 LANGUAGES C CXX)

include(CheckIPOSupported)
include(FetchContent)

find_package(Threads REQUIRED)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(JIMOKOMI_ENABLE_NATIVE_OPTIMIZATION "Tune local builds for the current machine" ON)
option(JIMOKOMI_ENABLE_IPO "Enable link-time optimization for supported builds" OFF)
option(JIMOKOMI_ENABLE_TRACY_PROFILE "Reserve a profiling build mode for future Tracy integration" OFF)

if(JIMOKOMI_ENABLE_NATIVE_OPTIMIZATION)
	if(MSVC)
		add_compile_options(/favor:INTEL64)
	else()
		add_compile_options(-march=native -mtune=native)
	endif()
endif()

if(JIMOKOMI_ENABLE_IPO)
	check_ipo_supported(RESULT jimokomi_ipo_supported OUTPUT jimokomi_ipo_output)
	if(NOT jimokomi_ipo_supported)
		message(FATAL_ERROR "IPO/LTO requested but not supported: ${jimokomi_ipo_output}")
	endif()
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

set(COREPHYS_SAMPLES OFF CACHE BOOL "" FORCE)
set(COREPHYS_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(COREPHYS_DOCS OFF CACHE BOOL "" FORCE)
set(COREPHYS_PROFILE ${JIMOKOMI_ENABLE_TRACY_PROFILE} CACHE BOOL "" FORCE)
set(COREPHYS_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(COREPHYS_VALIDATE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
set(OPENGL_VERSION "2.1" CACHE STRING "" FORCE)

if(UNIX AND NOT APPLE)
	set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
endif()

if(JIMOKOMI_ENABLE_TRACY_PROFILE)
	FetchContent_Declare(
		tracy
		GIT_REPOSITORY https://github.com/wolfpld/tracy.git
		GIT_TAG v0.11.1
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
	)
	FetchContent_MakeAvailable(tracy)
endif()

add_subdirectory(third_party/corephys)
add_subdirectory(third_party/raylib)

set(SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/main.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/App.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/Input.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/InputPacketStream.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/Memory.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/PathResolver.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/PlatformRuntime.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/Profiling.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/ProfilingOpenGL.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/Stats.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/TaskSystem.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Engine.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Physics/PhysicsBodies.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Physics/PhysicsParticles.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Physics/PhysicsQueries.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Physics/PhysicsTilemap.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Physics/PhysicsWorld.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Physics/PhysicsWorldSnapshot.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/Camera.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlay.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayCommon.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayDashboard.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayHistory.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayInput.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayInspector.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayLayout.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayPanels.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayStats.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/DebugOverlayWorld.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/GeneratedFrame.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/GridBackdrop.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ParticleVisualResources.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/RaylibBackend.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/RaylibBackendInstancing.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/Renderer.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/RendererSignatures.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/RendererSurfaceBatch.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/RendererVisibility.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/RenderSnapshot.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManager.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManagerBakeCache.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManagerBakeKey.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManagerBakeProcess.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManagerBakeQueue.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManagerInvalidation.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManagerLifecycle.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManagerRegistry.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/ResourceManagerStats.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/SceneRenderSnapshot.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/Target.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Runtime/AppRenderLoop.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Runtime/AppSimulation.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Runtime/InteractionPacket.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Runtime/InteractionSimulation.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Runtime/InteractionSystem.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/RuntimeConfig.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Component.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Components/CameraTargetComponent.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Components/ColliderComponent.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Components/DraggableComponent.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Components/RandomForceComponent.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Components/RenderableComponent.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Components/RigidBodyComponent.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Components/SelectableComponent.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Components/TransformComponent.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Entity.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Scene.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SceneAccess.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SceneFactories.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SceneParticleVisuals.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SceneQueries.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SceneStorage.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SceneView.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SpatialGrid.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SpatialGridDiagnostics.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SpatialGridQueries.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Systems/CameraFollowSystem.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Systems/InputRoutingSystem.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Systems/PhysicsSyncSystem.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Systems/RandomForceSystem.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/Settings.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/BallVisualResources.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/Game.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/GameWorld.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/LiquidSourceSystem.c"
)

add_executable(jimokomi ${SOURCES})

target_include_directories(jimokomi PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/third_party/corephys/include"
)

target_link_libraries(jimokomi PRIVATE corephys raylib Threads::Threads)

if(UNIX AND NOT APPLE)
	target_compile_definitions(jimokomi PRIVATE _POSIX_C_SOURCE=200809L)
	target_link_libraries(jimokomi PRIVATE m)
endif()

if(MSVC)
	target_compile_definitions(jimokomi PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
	target_compile_options(jimokomi PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(JIMOKOMI_ENABLE_TRACY_PROFILE)
	target_compile_definitions(jimokomi PRIVATE JIMOKOMI_ENABLE_TRACY_PROFILE=1)
	target_link_libraries(jimokomi PRIVATE TracyClient)
endif()
