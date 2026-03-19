project(TrinityEngine LANGUAGES CXX)

function(trinity_set_cache_bool name value)
    set(${name} ${value} CACHE BOOL "" FORCE)
endfunction()

find_package(Vulkan REQUIRED)
if(NOT TARGET Vulkan::glslc)
    message(FATAL_ERROR "Vulkan::glslc not found. Please make sure the Vulkan SDK is installed correctly.")
endif()

# Vendor libraries (subdirectories)
block()
    set(BUILD_SHARED_LIBS OFF)
    trinity_set_cache_bool(SPDLOG_BUILD_TESTS OFF)
    trinity_set_cache_bool(SPDLOG_BUILD_EXAMPLE OFF)
    trinity_set_cache_bool(SPDLOG_BUILD_BENCH OFF)
    trinity_set_cache_bool(SPDLOG_BUILD_SHARED OFF)
    add_subdirectory(vendor/spdlog)
endblock()

block()
    set(BUILD_TESTING OFF)
    trinity_set_cache_bool(YAML_CPP_BUILD_TOOLS OFF)
    trinity_set_cache_bool(YAML_CPP_BUILD_TESTS OFF)
    trinity_set_cache_bool(YAML_CPP_INSTALL OFF)
    add_subdirectory(vendor/yaml-cpp)
endblock()
if(MSVC)
    target_compile_options(yaml-cpp PRIVATE /wd4267)
    set_property(TARGET yaml-cpp PROPERTY COMPILE_WARNING_AS_ERROR OFF)
endif()

trinity_set_cache_bool(GLM_TEST_ENABLE OFF)
trinity_set_cache_bool(GLM_PERF_TEST_ENABLE OFF)
trinity_set_cache_bool(GLM_BUILD_LIBRARY OFF)
add_subdirectory(vendor/glm)

trinity_set_cache_bool(ENTT_BUILD_TESTING OFF)
trinity_set_cache_bool(ENTT_BUILD_DOCS OFF)
add_subdirectory(vendor/entt)

block()
    trinity_set_cache_bool(SDL_AUDIO OFF)
    trinity_set_cache_bool(SDL_CAMERA OFF)
    trinity_set_cache_bool(SDL_DIALOG OFF)
    trinity_set_cache_bool(SDL_DISKAUDIO OFF)
    trinity_set_cache_bool(SDL_DUMMYAUDIO OFF)
    trinity_set_cache_bool(SDL_DUMMYVIDEO OFF)
    trinity_set_cache_bool(SDL_EXAMPLES OFF)
    trinity_set_cache_bool(SDL_GPU OFF)
    trinity_set_cache_bool(SDL_HAPTIC OFF)
    trinity_set_cache_bool(SDL_HIDAPI ON)
    trinity_set_cache_bool(SDL_HIDAPI_JOYSTICK ON)
    trinity_set_cache_bool(SDL_JOYSTICK ON)
    trinity_set_cache_bool(SDL_OFFSCREEN OFF)
    trinity_set_cache_bool(SDL_OPENGL OFF)
    trinity_set_cache_bool(SDL_OPENGLES OFF)
    trinity_set_cache_bool(SDL_RENDER OFF)
    trinity_set_cache_bool(SDL_SENSOR OFF)
    trinity_set_cache_bool(SDL_TEST_LIBRARY OFF)
    trinity_set_cache_bool(SDL_TESTS OFF)
    trinity_set_cache_bool(SDL_VIDEO ON)
    trinity_set_cache_bool(SDL_VIRTUAL_JOYSTICK OFF)
    trinity_set_cache_bool(SDL_VULKAN ON)
    add_subdirectory(vendor/SDL)
endblock()

block()
    trinity_set_cache_bool(ASSIMP_BUILD_TESTS OFF)
    trinity_set_cache_bool(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
    trinity_set_cache_bool(ASSIMP_BUILD_SAMPLES OFF)
    trinity_set_cache_bool(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
    trinity_set_cache_bool(ASSIMP_IGNORE_GIT_HASH ON)
    trinity_set_cache_bool(ASSIMP_INSTALL OFF)
    trinity_set_cache_bool(ASSIMP_NO_EXPORT ON)
    add_subdirectory(vendor/assimp)
endblock()

add_subdirectory(vendor/imgui)
add_subdirectory(vendor/stb)
add_subdirectory(vendor/VulkanMemoryAllocator)
add_subdirectory(vendor/imguizmo)

#add_compile_definitions(TR_ENABLE_SHADER_COMPILATION)

set(TRINITY_ENGINE_SOURCES
    src/Trinity/Application/Application.cpp
    src/Trinity/Application/Application.h
    src/Trinity/Assets/AssetHandle.h
    src/Trinity/Assets/AssetManager.cpp
    src/Trinity/Assets/AssetManager.h
    src/Trinity/Assets/BuiltInAssets.cpp
    src/Trinity/Assets/BuiltInAssets.h
    src/Trinity/Assets/MeshAsset.cpp
    src/Trinity/Assets/MeshAsset.h
    src/Trinity/Assets/AssetRegistry.cpp
    src/Trinity/Assets/AssetRegistry.h
    src/Trinity/Camera/Camera.h
    src/Trinity/Camera/EditorCamera.cpp
    src/Trinity/Camera/EditorCamera.h
    src/Trinity/Core/EntryPoint.h
    src/Trinity/Core/stb.cpp
    src/Trinity/ECS/Components.h
    src/Trinity/ECS/Entity.h
    src/Trinity/ECS/Scene.cpp
    src/Trinity/ECS/Scene.h
    src/Trinity/ECS/SceneRenderer.cpp
    src/Trinity/ECS/SceneRenderer.h
    src/Trinity/ECS/SceneSerializer.cpp
    src/Trinity/ECS/SceneSerializer.h
    src/Trinity/Events/ApplicationEvent.h
    src/Trinity/Events/Event.h
    src/Trinity/Events/EventQueue.cpp
    src/Trinity/Events/EventQueue.h
    src/Trinity/Events/GamepadEvent.h
    src/Trinity/Events/KeyEvent.h
    src/Trinity/Events/MouseEvent.h
    src/Trinity/Geometry/Geometry.h
    src/Trinity/ImGui/ImGuiLayer.cpp
    src/Trinity/ImGui/ImGuiLayer.h
    src/Trinity/Input/Input.cpp
    src/Trinity/Input/Input.h
    src/Trinity/Input/InputCodes.h
    src/Trinity/Layer/Layer.cpp
    src/Trinity/Layer/Layer.h
    src/Trinity/Layer/LayerStack.cpp
    src/Trinity/Layer/LayerStack.h
    src/Trinity/Platform/SDL/SDLEventTranslator.cpp
    src/Trinity/Platform/SDL/SDLEventTranslator.h
    src/Trinity/Platform/Window.cpp
    src/Trinity/Platform/Window.h
    src/Trinity/Platform/Windows/WindowsWindow.cpp
    src/Trinity/Platform/Windows/WindowsWindow.h
    src/Trinity/Renderer/Buffer.cpp
    src/Trinity/Renderer/Buffer.h
    src/Trinity/Renderer/DirectX/DirectXRenderer.cpp
    src/Trinity/Renderer/DirectX/DirectXRenderer.h
    src/Trinity/Renderer/Material.cpp
    src/Trinity/Renderer/Material.h
    src/Trinity/Renderer/MoltenVK/MoltenVKRenderer.cpp
    src/Trinity/Renderer/MoltenVK/MoltenVKRenderer.h
    src/Trinity/Renderer/RenderCommand.cpp
    src/Trinity/Renderer/RenderCommand.h
    src/Trinity/Renderer/Renderer.h
    src/Trinity/Renderer/RendererFactory.cpp
    src/Trinity/Renderer/RendererFactory.h
    src/Trinity/Renderer/Shader.h
    src/Trinity/Renderer/ShaderLibrary.cpp
    src/Trinity/Renderer/ShaderLibrary.h
    src/Trinity/Renderer/Texture.h
    src/Trinity/Renderer/Vulkan/VulkanAllocator.cpp
    src/Trinity/Renderer/Vulkan/VulkanAllocator.h
    src/Trinity/Renderer/Vulkan/VulkanBuffer.cpp
    src/Trinity/Renderer/Vulkan/VulkanBuffer.h
    src/Trinity/Renderer/Vulkan/VulkanCommand.cpp
    src/Trinity/Renderer/Vulkan/VulkanCommand.h
    src/Trinity/Renderer/Vulkan/VulkanContext.cpp
    src/Trinity/Renderer/Vulkan/VulkanContext.h
    src/Trinity/Renderer/Vulkan/VulkanDescriptorAllocator.cpp
    src/Trinity/Renderer/Vulkan/VulkanDescriptorAllocator.h
    src/Trinity/Renderer/Vulkan/VulkanDevice.cpp
    src/Trinity/Renderer/Vulkan/VulkanDevice.h
    src/Trinity/Renderer/Vulkan/VulkanGeometryBuffer.cpp
    src/Trinity/Renderer/Vulkan/VulkanGeometryBuffer.h
    src/Trinity/Renderer/Vulkan/VulkanPipeline.cpp
    src/Trinity/Renderer/Vulkan/VulkanPipeline.h
    src/Trinity/Renderer/Vulkan/VulkanRenderer.cpp
    src/Trinity/Renderer/Vulkan/VulkanRenderer.h
    src/Trinity/Renderer/Vulkan/VulkanResourceStateTracker.cpp
    src/Trinity/Renderer/Vulkan/VulkanResourceStateTracker.h
    src/Trinity/Renderer/Vulkan/VulkanShaderInterop.h
    src/Trinity/Renderer/Vulkan/VulkanSurface.cpp
    src/Trinity/Renderer/Vulkan/VulkanSurface.h
    src/Trinity/Renderer/Vulkan/VulkanSwapchain.cpp
    src/Trinity/Renderer/Vulkan/VulkanSwapchain.h
    src/Trinity/Renderer/Vulkan/VulkanSync.cpp
    src/Trinity/Renderer/Vulkan/VulkanSync.h
    src/Trinity/Renderer/Vulkan/VulkanTexture.cpp
    src/Trinity/Renderer/Vulkan/VulkanTexture.h
    src/Trinity/Renderer/Vulkan/VulkanUploadContext.cpp
    src/Trinity/Renderer/Vulkan/VulkanUploadContext.h
    src/Trinity/Utilities/FileManagement.cpp
    src/Trinity/Utilities/FileManagement.h
    src/Trinity/Utilities/Log.cpp
    src/Trinity/Utilities/Log.h
    src/Trinity/Utilities/Time.cpp
    src/Trinity/Utilities/Time.h
    src/Trinity/Utilities/VulkanUtilities.cpp
    src/Trinity/Utilities/VulkanUtilities.h
)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE ${TRINITY_ENGINE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    spdlog::spdlog
    glm::glm
    EnTT::EnTT
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    SDL3::SDL3
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    yaml-cpp::yaml-cpp
    imgui
    stb::stb
    assimp
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    $<$<CONFIG:Debug>:TRINITY_DEBUG>
    $<$<CONFIG:Release>:TRINITY_RELEASE>
)

if (WIN32 AND TRINITY_USE_VULKAN_WIN32_PLATFORM)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_WIN32_KHR)
endif()

option(TRINITY_EMBED_SHADERS "Embed SPIR-V shaders into binary instead of runtime files" OFF)

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders)
set(SHADER_BUILD_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders)
file(MAKE_DIRECTORY ${SHADER_BUILD_OUTPUT_DIR})

set(TRINITY_SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/GeometryBuffer.frag
    ${SHADER_SOURCE_DIR}/GeometryBuffer.vert
    ${SHADER_SOURCE_DIR}/Lighting.frag
    ${SHADER_SOURCE_DIR}/Lighting.vert
    ${SHADER_SOURCE_DIR}/Shadow.frag
    ${SHADER_SOURCE_DIR}/Shadow.vert
    ${SHADER_SOURCE_DIR}/Simple.frag
    ${SHADER_SOURCE_DIR}/Simple.vert
)

set(TRINITY_SHADER_INCLUDE_SOURCES
    ${SHADER_SOURCE_DIR}/ColorOutput.glsl
    ${SHADER_SOURCE_DIR}/PBR.glsl
)

set(ALL_SPV_OUTPUTS "")
set(GLSLC_FLAGS --target-env=vulkan1.4 -I ${SHADER_SOURCE_DIR})

foreach(SHADER_FILE IN LISTS TRINITY_SHADER_SOURCES)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SPV_OUTPUT ${SHADER_BUILD_OUTPUT_DIR}/${SHADER_NAME}.spv)

    add_custom_command(OUTPUT ${SPV_OUTPUT}
        COMMAND Vulkan::glslc ${GLSLC_FLAGS} -o ${SPV_OUTPUT} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE} ${TRINITY_SHADER_INCLUDE_SOURCES}
        COMMENT "COMPILING SHADER: ${SHADER_NAME}"
        VERBATIM
    )

    list(APPEND ALL_SPV_OUTPUTS "${SPV_OUTPUT}")
endforeach()

add_custom_target(TRINITY_SHADERS
    DEPENDS ${ALL_SPV_OUTPUTS}
    SOURCES ${TRINITY_SHADER_SOURCES} ${TRINITY_SHADER_INCLUDE_SOURCES}
)

set_property(GLOBAL PROPERTY TRINITY_SHADER_OUTPUTS "${ALL_SPV_OUTPUTS}")

message(STATUS "Engine shaders will be compiled to: ${SHADER_BUILD_OUTPUT_DIR}")
