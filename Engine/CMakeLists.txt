project(Engine)

find_package(Vulkan REQUIRED)

set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/spdlog)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)

set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/yaml-cpp)
if (MSVC)
    target_compile_options(yaml-cpp PRIVATE /wd4267)
    set_property(TARGET yaml-cpp PROPERTY COMPILE_WARNING_AS_ERROR OFF)
endif()

set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
set(GLM_PERF_TEST_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glm)

set(ENTT_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENTT_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/entt)

add_subdirectory(vendor/imgui)
add_subdirectory(vendor/stb)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${ENGINE_SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    spdlog::spdlog
    glfw
    glm::glm
    EnTT::EnTT
    yaml-cpp::yaml-cpp
    Vulkan::Vulkan
    imgui
    stb::stb
)