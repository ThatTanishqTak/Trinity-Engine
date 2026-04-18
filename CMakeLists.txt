cmake_minimum_required(VERSION 3.30)
project(Trinity)

set(Trinity_Platform "Desktop" CACHE STRING "")
set_property(CACHE Trinity_Platform PROPERTY STRINGS Desktop Xbox PlayStation Nintendo)

set(Trinity_Renderer "Vulkan" CACHE STRING "")
set_property(CACHE Trinity_Renderer PROPERTY STRINGS Vulkan DirectX12 Metal)

option(Trinity_BuildForge "Build TrinityForge" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})
if(ARCH_NAME MATCHES AMD64|x86_64)
    set(ARCH_NAME x86_64)
elseif(ARCH_NAME MATCHES i386|i686)
    set(ARCH_NAME x86_32)
endif()

set(OUTPUT_BASE ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${CMAKE_SYSTEM_NAME}-${ARCH_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BASE}/${PROJECT_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BASE}/${PROJECT_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BASE}/${PROJECT_NAME})

add_library(TrinityConfig INTERFACE)
target_compile_features(TrinityConfig INTERFACE cxx_std_23)

if(MSVC)
    target_compile_options(TrinityConfig INTERFACE /MP /EHsc)
endif()

target_compile_definitions(TrinityConfig INTERFACE
    $<$<CONFIG:Debug>:TRINITY_DEBUG>
    $<$<CONFIG:Release>:TRINITY_RELEASE>
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

if(Trinity_Platform STREQUAL "Desktop")
    target_compile_definitions(TrinityConfig INTERFACE TRINITY_PLATFORM_DESKTOP)
elseif(Trinity_Platform STREQUAL "Xbox")
    target_compile_definitions(TrinityConfig INTERFACE TRINITY_PLATFORM_XBOX)
elseif(Trinity_Platform STREQUAL "PlayStation")
    target_compile_definitions(TrinityConfig INTERFACE TRINITY_PLATFORM_PLAYSTATION)
elseif(Trinity_Platform STREQUAL "Nintendo")
    target_compile_definitions(TrinityConfig INTERFACE TRINITY_PLATFORM_NINTENDO)
else()
    message(FATAL_ERROR "Invalid Trinity_Platform: ${Trinity_Platform}")
endif()

if(APPLE AND Trinity_Platform STREQUAL "Desktop")
    target_compile_definitions(TrinityConfig INTERFACE TRINITY_PLATFORM_MACOS)
endif()

if(Trinity_Renderer STREQUAL "Vulkan")
    target_compile_definitions(TrinityConfig INTERFACE TRINITY_RENDERER_VULKAN)
elseif(Trinity_Renderer STREQUAL "DirectX12")
    target_compile_definitions(TrinityConfig INTERFACE TRINITY_RENDERER_DIRECTX12)
elseif(Trinity_Renderer STREQUAL "Metal")
    target_compile_definitions(TrinityConfig INTERFACE TRINITY_RENDERER_METAL)
else()
    message(FATAL_ERROR "Invalid Trinity_Renderer: ${Trinity_Renderer}")
endif()

if(Trinity_Platform STREQUAL "Desktop" AND Trinity_Renderer STREQUAL "Vulkan")
    if(WIN32)
        set(TRINITY_USE_VULKAN_WIN32_PLATFORM ON CACHE INTERNAL "" FORCE)
    elseif(APPLE)
        set(TRINITY_USE_VULKAN_MACOS_PLATFORM ON CACHE INTERNAL "" FORCE)
    endif()
endif()

if(Trinity_Platform STREQUAL "Desktop")
    if(NOT Trinity_Renderer STREQUAL "Vulkan")
        message(FATAL_ERROR "Desktop currently supports only Vulkan.")
    endif()
elseif(Trinity_Platform STREQUAL "Xbox")
    if(NOT Trinity_Renderer STREQUAL "DirectX12")
        message(FATAL_ERROR "Xbox requires DirectX12.")
    endif()
    message(FATAL_ERROR "Xbox is not wired yet.")
elseif(Trinity_Platform STREQUAL "PlayStation")
    message(FATAL_ERROR "PlayStation is not wired yet.")
elseif(Trinity_Platform STREQUAL "Nintendo")
    message(FATAL_ERROR "Nintendo is not wired yet.")
endif()

add_subdirectory(Trinity-Engine)

if(Trinity_BuildForge)
    if(NOT Trinity_Platform STREQUAL "Desktop")
        message(FATAL_ERROR "TrinityForge is desktop-only.")
    endif()
    add_subdirectory(Trinity-Forge)
endif()
