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
        glm
        EnTT
        imgui
    PRIVATE
        Slang
        stb
        assimp
        miniaudio
)

target_include_directories(Trinity-Engine PRIVATE "${CMAKE_SOURCE_DIR}/Vendor/imgui/backends")

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

if(TRINITY_ENABLE_VULKAN)
    target_link_libraries(Trinity-Engine
        PRIVATE
            Vulkan::Vulkan
            VulkanMemoryAllocator
    )

    target_compile_definitions(Trinity-Engine
        PRIVATE
            TRINITY_ENABLE_VULKAN
    )
endif()

add_library(Trinity::Engine ALIAS Trinity-Engine)

if(TRINITY_ENABLE_VULKAN)
    set(TRINITY_ENGINE_SHADER_DIR "${TRINITY_ENGINE_ROOT}/Shaders")
    set(TRINITY_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Shaders")

    set(TRINITY_ENGINE_SLANG_SHADERS
        "${TRINITY_ENGINE_SHADER_DIR}/Triangle.slang"
        "${TRINITY_ENGINE_SHADER_DIR}/Mesh.slang"
        "${TRINITY_ENGINE_SHADER_DIR}/Tonemap.slang"
        "${TRINITY_ENGINE_SHADER_DIR}/DepthVisualize.slang"
        "${TRINITY_ENGINE_SHADER_DIR}/Skybox.slang"
        "${TRINITY_ENGINE_SHADER_DIR}/IrradianceConvolve.slang"
        "${TRINITY_ENGINE_SHADER_DIR}/Prefilter.slang"
        "${TRINITY_ENGINE_SHADER_DIR}/BrdfLut.slang"
    )

    file(MAKE_DIRECTORY "${TRINITY_SHADER_OUTPUT_DIR}")

    set(TRINITY_COMPILED_SHADERS "")
    foreach(_slang ${TRINITY_ENGINE_SLANG_SHADERS})
        get_filename_component(_slangname "${_slang}" NAME)
        set(_staged "${TRINITY_SHADER_OUTPUT_DIR}/${_slangname}")

        add_custom_command(
            OUTPUT "${_staged}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_slang}" "${_staged}"
            DEPENDS "${_slang}"
            COMMENT "Staging Slang shader ${_slangname}"
            VERBATIM
        )

        list(APPEND TRINITY_COMPILED_SHADERS "${_staged}")
    endforeach()

    add_custom_target(Trinity-Shaders
        DEPENDS ${TRINITY_COMPILED_SHADERS}
        SOURCES ${TRINITY_ENGINE_SLANG_SHADERS}
    )

    trinity_set_ide_folder(Trinity-Shaders "Trinity/Engine")
endif()

trinity_set_ide_folder(Trinity-Engine "Trinity/Engine")