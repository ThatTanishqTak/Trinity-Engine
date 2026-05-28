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
trinity_set_ide_folder(spdlog "Trinity/Vendor")

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(Vendor/yaml-cpp)
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
message(STATUS "Output Base: ${CMAKE_SOURCE_DIR}/bin/<CONFIG>-${CMAKE_SYSTEM_NAME}-${ARCH_NAME}/<TARGET>")