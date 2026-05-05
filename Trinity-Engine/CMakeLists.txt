project(TrinityEngine LANGUAGES CXX)

function(vendor_build_option name value)
    set(${name} ${value} CACHE BOOL "" FORCE)
endfunction()

function(trinity_filter_sources out_var)
    set(local_sources ${ARGN})

    if(Trinity_Platform STREQUAL "Desktop")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Window/(Xbox|XBox|PlayStation|Nintendo)/.*")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Input/(Xbox|XBox|PlayStation|Nintendo)/.*")
    elseif(Trinity_Platform STREQUAL "Xbox")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Window/(Desktop|PlayStation|Nintendo)/.*")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Input/(Desktop|PlayStation|Nintendo)/.*")
    elseif(Trinity_Platform STREQUAL "PlayStation")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Window/(Desktop|Xbox|XBox|Nintendo)/.*")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Input/(Desktop|Xbox|XBox|Nintendo)/.*")
    elseif(Trinity_Platform STREQUAL "Nintendo")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Window/(Desktop|Xbox|XBox|PlayStation)/.*")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Input/(Desktop|Xbox|XBox|PlayStation)/.*")
    endif()

    if(Trinity_Renderer STREQUAL "Vulkan")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Renderer/(DirectX12|Metal)/.*")
    elseif(Trinity_Renderer STREQUAL "DirectX12")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Renderer/Vulkan/.*")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Renderer/Metal/.*")
    elseif(Trinity_Renderer STREQUAL "Metal")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Renderer/Vulkan/.*")
        list(FILTER local_sources EXCLUDE REGEX ".*/Platform/Renderer/DirectX12/.*")
    endif()

    set(${out_var} ${local_sources} PARENT_SCOPE)
endfunction()

if(Trinity_Renderer STREQUAL "Vulkan")
    find_package(Vulkan REQUIRED)
    if(NOT TARGET Vulkan::glslc)
        message(FATAL_ERROR "Vulkan::glslc not found")
    endif()
endif()

block()
    set(BUILD_SHARED_LIBS OFF)
    vendor_build_option(SPDLOG_BUILD_TESTS OFF)
    vendor_build_option(SPDLOG_BUILD_EXAMPLE OFF)
    vendor_build_option(SPDLOG_BUILD_BENCH OFF)
    vendor_build_option(SPDLOG_BUILD_SHARED OFF)
    add_subdirectory(vendor/spdlog)
endblock()

block()
    set(BUILD_TESTING OFF)
    vendor_build_option(YAML_CPP_BUILD_TOOLS OFF)
    vendor_build_option(YAML_CPP_BUILD_TESTS OFF)
    vendor_build_option(YAML_CPP_INSTALL OFF)
    add_subdirectory(vendor/yaml-cpp)
endblock()
if(MSVC)
    target_compile_options(yaml-cpp PRIVATE /wd4267)
    set_property(TARGET yaml-cpp PROPERTY COMPILE_WARNING_AS_ERROR OFF)
endif()

vendor_build_option(GLM_TEST_ENABLE OFF)
vendor_build_option(GLM_PERF_TEST_ENABLE OFF)
vendor_build_option(GLM_BUILD_LIBRARY OFF)
add_subdirectory(vendor/glm)

vendor_build_option(ENTT_BUILD_TESTING OFF)
vendor_build_option(ENTT_BUILD_DOCS OFF)
add_subdirectory(vendor/entt)

block()
    vendor_build_option(SDL_AUDIO OFF)
    vendor_build_option(SDL_CAMERA OFF)
    vendor_build_option(SDL_DIALOG OFF)
    vendor_build_option(SDL_DISKAUDIO OFF)
    vendor_build_option(SDL_DUMMYAUDIO OFF)
    vendor_build_option(SDL_DUMMYVIDEO OFF)
    vendor_build_option(SDL_EXAMPLES OFF)
    vendor_build_option(SDL_GPU OFF)
    vendor_build_option(SDL_HAPTIC OFF)
    vendor_build_option(SDL_OFFSCREEN OFF)
    vendor_build_option(SDL_OPENGL OFF)
    vendor_build_option(SDL_OPENGLES OFF)
    vendor_build_option(SDL_RENDER OFF)
    vendor_build_option(SDL_SENSOR OFF)
    vendor_build_option(SDL_TEST_LIBRARY OFF)
    vendor_build_option(SDL_TESTS OFF)
    vendor_build_option(SDL_VIRTUAL_JOYSTICK OFF)

    if(Trinity_Platform STREQUAL "Desktop")
        vendor_build_option(SDL_HIDAPI ON)
        vendor_build_option(SDL_HIDAPI_JOYSTICK ON)
        vendor_build_option(SDL_JOYSTICK ON)
        vendor_build_option(SDL_VIDEO ON)
    else()
        vendor_build_option(SDL_HIDAPI OFF)
        vendor_build_option(SDL_HIDAPI_JOYSTICK OFF)
        vendor_build_option(SDL_JOYSTICK OFF)
        vendor_build_option(SDL_VIDEO OFF)
    endif()

    if(Trinity_Platform STREQUAL "Desktop" AND Trinity_Renderer STREQUAL "Vulkan")
        vendor_build_option(SDL_VULKAN ON)
    else()
        vendor_build_option(SDL_VULKAN OFF)
    endif()

    add_subdirectory(vendor/SDL)
endblock()

block()
    vendor_build_option(ASSIMP_BUILD_TESTS OFF)
    vendor_build_option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
    vendor_build_option(ASSIMP_BUILD_SAMPLES OFF)
    vendor_build_option(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
    vendor_build_option(ASSIMP_IGNORE_GIT_HASH ON)
    vendor_build_option(ASSIMP_INSTALL OFF)
    vendor_build_option(ASSIMP_NO_EXPORT ON)
    add_subdirectory(vendor/assimp)
endblock()

add_subdirectory(vendor/imgui)
add_subdirectory(vendor/stb)
add_subdirectory(vendor/imguizmo)

if(Trinity_Renderer STREQUAL "Vulkan")
    add_subdirectory(vendor/VulkanMemoryAllocator)
endif()

if(Trinity_Renderer STREQUAL "Vulkan")
    set(TRINITY_SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source)
    set(TRINITY_SHADER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/compiled)

    file(MAKE_DIRECTORY ${TRINITY_SHADER_OUTPUT_DIR})

    file(GLOB_RECURSE TRINITY_SHADER_SOURCES CONFIGURE_DEPENDS
        ${TRINITY_SHADER_SOURCE_DIR}/*.glsl
    )

    function(trinity_compile_shader SHADER_SOURCE OUTPUT_SPV STAGE)
        add_custom_command(
            OUTPUT ${OUTPUT_SPV}
            COMMAND Vulkan::glslc -fshader-stage=${STAGE} ${SHADER_SOURCE} -o ${OUTPUT_SPV}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling shader ${SHADER_SOURCE}"
        )
    endfunction()

    set(TRINITY_COMPILED_SHADERS "")
    foreach(TRINITY_SHADER ${TRINITY_SHADER_SOURCES})
        get_filename_component(TRINITY_SHADER_FILENAME ${TRINITY_SHADER} NAME)

        if(TRINITY_SHADER MATCHES "\\.vert\\.glsl$")
            string(REPLACE ".vert.glsl" ".vert.spv" TRINITY_SHADER_OUTPUT_NAME "${TRINITY_SHADER_FILENAME}")
            trinity_compile_shader(${TRINITY_SHADER} ${TRINITY_SHADER_OUTPUT_DIR}/${TRINITY_SHADER_OUTPUT_NAME} vertex)
            list(APPEND TRINITY_COMPILED_SHADERS ${TRINITY_SHADER_OUTPUT_DIR}/${TRINITY_SHADER_OUTPUT_NAME})
        elseif(TRINITY_SHADER MATCHES "\\.frag\\.glsl$")
            string(REPLACE ".frag.glsl" ".frag.spv" TRINITY_SHADER_OUTPUT_NAME "${TRINITY_SHADER_FILENAME}")
            trinity_compile_shader(${TRINITY_SHADER} ${TRINITY_SHADER_OUTPUT_DIR}/${TRINITY_SHADER_OUTPUT_NAME} fragment)
            list(APPEND TRINITY_COMPILED_SHADERS ${TRINITY_SHADER_OUTPUT_DIR}/${TRINITY_SHADER_OUTPUT_NAME})
        elseif(TRINITY_SHADER MATCHES "\\.comp\\.glsl$")
            string(REPLACE ".comp.glsl" ".comp.spv" TRINITY_SHADER_OUTPUT_NAME "${TRINITY_SHADER_FILENAME}")
            trinity_compile_shader(${TRINITY_SHADER} ${TRINITY_SHADER_OUTPUT_DIR}/${TRINITY_SHADER_OUTPUT_NAME} compute)
            list(APPEND TRINITY_COMPILED_SHADERS ${TRINITY_SHADER_OUTPUT_DIR}/${TRINITY_SHADER_OUTPUT_NAME})
        endif()
    endforeach()

    add_custom_target(TrinityShaders DEPENDS ${TRINITY_COMPILED_SHADERS})
endif()

file(GLOB_RECURSE TRINITY_ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

trinity_filter_sources(TRINITY_ENGINE_FILTERED_SOURCES ${TRINITY_ENGINE_SOURCES})

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE ${TRINITY_ENGINE_FILTERED_SOURCES})

if(TARGET TrinityShaders)
    add_dependencies(${PROJECT_NAME} TrinityShaders)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    TrinityConfig
    spdlog::spdlog
    glm::glm
    EnTT::EnTT
    SDL3::SDL3
)

if(Trinity_Renderer STREQUAL "Vulkan")
    target_link_libraries(${PROJECT_NAME} PUBLIC
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        TRINITY_SHADER_DIR="${TRINITY_SHADER_OUTPUT_DIR}/"
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    yaml-cpp::yaml-cpp
    imgui
    stb::stb
    assimp
)