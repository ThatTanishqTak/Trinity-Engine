project(TrinityEngine LANGUAGES CXX)

function(vendor_build_option name value)
    set(${name} ${value} CACHE BOOL "" FORCE)
endfunction()

find_package(Vulkan REQUIRED)
if(NOT TARGET Vulkan::glslc)
    message(FATAL_ERROR "Vulkan::glslc not found")
endif()

# Vendor libraries (subdirectories)
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
    vendor_build_option(SDL_HIDAPI ON)
    vendor_build_option(SDL_HIDAPI_JOYSTICK ON)
    vendor_build_option(SDL_JOYSTICK ON)
    vendor_build_option(SDL_OFFSCREEN OFF)
    vendor_build_option(SDL_OPENGL OFF)
    vendor_build_option(SDL_OPENGLES OFF)
    vendor_build_option(SDL_RENDER OFF)
    vendor_build_option(SDL_SENSOR OFF)
    vendor_build_option(SDL_TEST_LIBRARY OFF)
    vendor_build_option(SDL_TESTS OFF)
    vendor_build_option(SDL_VIDEO ON)
    vendor_build_option(SDL_VIRTUAL_JOYSTICK OFF)
    vendor_build_option(SDL_VULKAN ON)
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
add_subdirectory(vendor/VulkanMemoryAllocator)
add_subdirectory(vendor/imguizmo)


file(GLOB_RECURSE TRINITY_ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
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