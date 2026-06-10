cmake_minimum_required(VERSION 3.30)

project(Trinity VERSION 0.1.0 DESCRIPTION "Trinity Game Engine Ecosystem" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(TrinityCommon)

trinity_detect_architecture(ARCH_NAME)

set(TRINITY_OUTPUT_BASE
    "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${CMAKE_SYSTEM_NAME}-${ARCH_NAME}/${PROJECT_NAME}"
)

option(TRINITY_BUILD_ENGINE "Build Trinity-Engine" ON)
option(TRINITY_BUILD_FORGE "Build Trinity-Forge editor" ON)
option(TRINITY_BUILD_RUNTIME "Build Trinity-Runtime player" ON)
option(TRINITY_BUILD_SERVER "Build Trinity-Server headless runtime" ON)
option(TRINITY_BUILD_HUB "Build Trinity-Hub launcher" ON)
option(TRINITY_BUILD_TOOLS "Build Trinity command-line tools" ON)

option(TRINITY_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

option(TRINITY_ENABLE_VULKAN "Enable Vulkan backend" ON)
option(TRINITY_ENABLE_METAL "Enable Metal backend" OFF)
option(TRINITY_ENABLE_DIRECTX12 "Enable DirectX 12 backend" OFF)
option(TRINITY_ENABLE_SDL "Enable SDL3 platform backend" ON)

if(WIN32)
    set(TRINITY_PLATFORM_DEFINE TRINITY_PLATFORM_WINDOWS)
elseif(APPLE)
    set(TRINITY_PLATFORM_DEFINE TRINITY_PLATFORM_MACOS)
elseif(UNIX)
    set(TRINITY_PLATFORM_DEFINE TRINITY_PLATFORM_LINUX)
else()
    set(TRINITY_PLATFORM_DEFINE TRINITY_PLATFORM_UNKNOWN)
endif()

if((TRINITY_BUILD_FORGE OR TRINITY_BUILD_RUNTIME OR TRINITY_BUILD_SERVER OR TRINITY_BUILD_HUB OR TRINITY_BUILD_TOOLS)
   AND NOT TRINITY_BUILD_ENGINE)
    message(FATAL_ERROR "Trinity products/tools require TRINITY_BUILD_ENGINE=ON.")
endif()

set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(Vendor/spdlog)
target_include_directories(spdlog SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/Vendor/spdlog/include")
trinity_set_ide_folder(spdlog "Trinity/Vendor")

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(Vendor/yaml-cpp)
target_include_directories(yaml-cpp SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/Vendor/yaml-cpp/include")
trinity_set_ide_folder(yaml-cpp "Trinity/Vendor")

if(TRINITY_ENABLE_SDL)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

    add_subdirectory(Vendor/SDL)
    trinity_set_ide_folder(SDL3-static "Trinity/Vendor")
endif()

add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/Vendor/glm")
target_compile_definitions(glm INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)
trinity_set_ide_folder(glm "Trinity/Vendor")

if(TRINITY_ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)

    add_library(VulkanMemoryAllocator INTERFACE)
    target_include_directories(VulkanMemoryAllocator SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/Vendor/VulkanMemoryAllocator/include")
    target_link_libraries(VulkanMemoryAllocator INTERFACE Vulkan::Vulkan)

    trinity_set_ide_folder(VulkanMemoryAllocator "Trinity/Vendor")

    find_path(TRINITY_SLANG_INCLUDE_DIR
        NAMES slang.h
        PATHS "$ENV{VULKAN_SDK}/Include" "$ENV{VULKAN_SDK}/Include/slang"
        NO_DEFAULT_PATH
    )

    find_library(TRINITY_SLANG_LIBRARY
        NAMES slang
        PATHS "$ENV{VULKAN_SDK}/Lib"
        NO_DEFAULT_PATH
    )

    if(NOT TRINITY_SLANG_INCLUDE_DIR OR NOT TRINITY_SLANG_LIBRARY)
        message(FATAL_ERROR "Slang not found in the Vulkan SDK ($ENV{VULKAN_SDK}); requires SDK >= 1.3.296.")
    endif()

    add_library(Slang INTERFACE)
    target_include_directories(Slang SYSTEM INTERFACE "${TRINITY_SLANG_INCLUDE_DIR}")
    target_link_libraries(Slang INTERFACE "${TRINITY_SLANG_LIBRARY}")

    file(GLOB TRINITY_SLANG_RUNTIME_DLLS "$ENV{VULKAN_SDK}/Bin/slang*.dll")
endif()

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/Vendor/stb")

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/Vendor/miniaudio")
if(UNIX AND NOT APPLE)
    target_link_libraries(miniaudio INTERFACE ${CMAKE_DL_LIBS} pthread m)
elseif(APPLE)
    target_link_libraries(miniaudio INTERFACE "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
endif()
trinity_set_ide_folder(miniaudio "Trinity/Vendor")

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_3DS_IMPORTER ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(Vendor/assimp)
target_include_directories(assimp SYSTEM INTERFACE
    "${CMAKE_SOURCE_DIR}/Vendor/assimp/include"
    "${CMAKE_BINARY_DIR}/Vendor/assimp/include"
)
trinity_set_ide_folder(assimp "Trinity/Vendor")
if(TARGET zlibstatic)
    trinity_set_ide_folder(zlibstatic "Trinity/Vendor")
endif()

add_library(EnTT INTERFACE)
target_include_directories(EnTT SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/Vendor/entt/single_include")

add_library(imgui STATIC
    "${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui.cpp"
    "${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_draw.cpp"
    "${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_tables.cpp"
    "${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_widgets.cpp"
    "${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_demo.cpp"
    "${CMAKE_SOURCE_DIR}/Vendor/imgui/backends/imgui_impl_sdl3.cpp"
    "${CMAKE_SOURCE_DIR}/Vendor/imgui/backends/imgui_impl_vulkan.cpp"
    "${CMAKE_SOURCE_DIR}/Vendor/ImGuizmo/src/ImGuizmo.cpp"
)
target_include_directories(imgui SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/Vendor/imgui" "${CMAKE_SOURCE_DIR}/Vendor/ImGuizmo/src")
target_include_directories(imgui SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/Vendor/imgui/backends")
target_link_libraries(imgui PRIVATE SDL3-static Vulkan::Vulkan)
trinity_set_ide_folder(imgui "Trinity/Vendor")
target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32)

if(TRINITY_BUILD_ENGINE)
    add_subdirectory(Trinity-Engine)
endif()

if(TRINITY_BUILD_FORGE)
    add_subdirectory(Trinity-Forge)
endif()

if(TRINITY_BUILD_RUNTIME)
    add_subdirectory(Trinity-Runtime)
endif()

if(TRINITY_BUILD_SERVER)
    add_subdirectory(Trinity-Server)
endif()

if(TRINITY_BUILD_HUB)
    add_subdirectory(Trinity-Hub)
endif()

if(TRINITY_BUILD_TOOLS)
    add_subdirectory(Trinity-Tools)
endif()

message(STATUS "Trinity ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${ARCH_NAME}")
message(STATUS "Output Base: ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${CMAKE_SYSTEM_NAME}-${ARCH_NAME}/<TARGET>")