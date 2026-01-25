project(Engine LANGUAGES CXX)

find_package(Vulkan REQUIRED)
if(NOT TARGET Vulkan::glslc)
    message(FATAL_ERROR "Vulkan::glslc not found. Please make sure the Vulkan SDK is installed correctly.")
endif()

# Vendor libraries (subdirectories)
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
if(MSVC)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c
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

# Useful compile definitions / options
target_compile_definitions(${PROJECT_NAME} PUBLIC
    $<$<CONFIG:Debug>:ENGINE_DEBUG>
    $<$<CONFIG:Release>:ENGINE_RELEASE>
)

option(ENGINE_EMBED_SHADERS "Embed SPIR-V shaders into binary instead of runtime files" OFF)

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders)

set(SHADER_BUILD_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders)
file(MAKE_DIRECTORY ${SHADER_BUILD_OUTPUT_DIR})

file(GLOB_RECURSE SHADER_SOURCES CONFIGURE_DEPENDS
    ${SHADER_SOURCE_DIR}/*.vert
    ${SHADER_SOURCE_DIR}/*.frag
)

set(ALL_SPV_OUTPUTS "")
set(GLSLC_FLAGS --target-env=vulkan1.4)

foreach(SHADER_FILE IN LISTS SHADER_SOURCES)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SPV_OUTPUT ${SHADER_BUILD_OUTPUT_DIR}/${SHADER_NAME}.spv)

    add_custom_command(OUTPUT ${SPV_OUTPUT}
        COMMAND Vulkan::glslc ${GLSLC_FLAGS} -o ${SPV_OUTPUT} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling shader: ${SHADER_NAME}"
        VERBATIM
    )

    list(APPEND ALL_SPV_OUTPUTS "${SPV_OUTPUT}")
endforeach()

add_custom_target(EngineShaders ALL
    DEPENDS ${ALL_SPV_OUTPUTS}
    SOURCES ${SHADER_SOURCES}
)

add_dependencies(${PROJECT_NAME} EngineShaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets/Shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_BUILD_OUTPUT_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets/Shaders
    COMMENT "Copying compiled shaders to output"
    USES_TERMINAL
)

message(STATUS "Engine shaders will be compiled to: ${SHADER_BUILD_OUTPUT_DIR}")