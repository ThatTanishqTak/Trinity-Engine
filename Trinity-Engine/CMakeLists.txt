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
    if(NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
        message(FATAL_ERROR "glslangValidator not found in the Vulkan SDK; cannot compile shaders.")
    endif()

    set(TRINITY_ENGINE_SHADER_DIR "${TRINITY_ENGINE_ROOT}/Shaders")
    set(TRINITY_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Shaders")

    set(TRINITY_ENGINE_SHADERS
        "${TRINITY_ENGINE_SHADER_DIR}/Triangle.vert"
        "${TRINITY_ENGINE_SHADER_DIR}/Triangle.frag"
    )

    file(MAKE_DIRECTORY "${TRINITY_SHADER_OUTPUT_DIR}")

    set(TRINITY_COMPILED_SHADERS "")
    foreach(_shader ${TRINITY_ENGINE_SHADERS})
        get_filename_component(_name "${_shader}" NAME)
        set(_output "${TRINITY_SHADER_OUTPUT_DIR}/${_name}.spv")

        add_custom_command(
            OUTPUT "${_output}"
            COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" -V "${_shader}" -o "${_output}"
            DEPENDS "${_shader}"
            COMMENT "Compiling shader ${_name}"
            VERBATIM
        )

        list(APPEND TRINITY_COMPILED_SHADERS "${_output}")
    endforeach()

    add_custom_target(Trinity-Shaders
        DEPENDS ${TRINITY_COMPILED_SHADERS}
        SOURCES ${TRINITY_ENGINE_SHADERS}
    )

    trinity_set_ide_folder(Trinity-Shaders "Trinity/Engine")
endif()

trinity_set_ide_folder(Trinity-Engine "Trinity/Engine")