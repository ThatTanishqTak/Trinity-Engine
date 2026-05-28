set(TRINITY_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(TRINITY_ENGINE_SOURCE_DIR "${TRINITY_ENGINE_ROOT}/Source")
set(TRINITY_ENGINE_INCLUDE_DIR "${TRINITY_ENGINE_ROOT}/Include")

trinity_add_static_library(Trinity-Engine "${TRINITY_ENGINE_SOURCE_DIR}")

target_include_directories(Trinity-Engine
    PUBLIC
        "${TRINITY_ENGINE_INCLUDE_DIR}"
    PRIVATE
        "${TRINITY_ENGINE_SOURCE_DIR}"
)

target_compile_definitions(Trinity-Engine
    PUBLIC
        ${TRINITY_PLATFORM_DEFINE}
        $<$<BOOL:${TRINITY_ENABLE_VULKAN}>:TRINITY_ENABLE_VULKAN>
        $<$<BOOL:${TRINITY_ENABLE_METAL}>:TRINITY_ENABLE_METAL>
        $<$<BOOL:${TRINITY_ENABLE_DIRECTX12}>:TRINITY_ENABLE_DIRECTX12>
)

target_link_libraries(Trinity-Engine
    PUBLIC
        spdlog::spdlog
        yaml-cpp::yaml-cpp
)

if(TRINITY_ENABLE_SDL)
    target_link_libraries(Trinity-Engine
        PRIVATE
            SDL3::SDL3-static
    )

    target_compile_definitions(Trinity-Engine
        PRIVATE
            TRINITY_ENABLE_SDL
    )
endif()

add_library(Trinity::Engine ALIAS Trinity-Engine)

trinity_set_ide_folder(Trinity-Engine "Trinity/Engine")