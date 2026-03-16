project(TrinityForge)

set(TRINITY_FORGE_SOURCES
    src/Forge/Forge.cpp
    src/Forge/ForgeLayer.cpp
    src/Forge/ForgeLayer.h
)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${TRINITY_FORGE_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    TrinityEngine
    imgui
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_dependencies(${PROJECT_NAME} TRINITY_SHADERS)

get_property(TRINITY_SHADER_OUTPUTS GLOBAL PROPERTY TRINITY_SHADER_OUTPUTS)

set(TRINITY_SHADER_RUNTIME_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets/Shaders")
set(TRINITY_SHADER_COPY_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TRINITY_SHADER_RUNTIME_DIR}"
)

foreach(SPV_OUTPUT IN LISTS TRINITY_SHADER_OUTPUTS)
    get_filename_component(SHADER_BINARY_NAME "${SPV_OUTPUT}" NAME)
    list(APPEND TRINITY_SHADER_COPY_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SPV_OUTPUT}" "${TRINITY_SHADER_RUNTIME_DIR}/${SHADER_BINARY_NAME}"
    )
endforeach()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    ${TRINITY_SHADER_COPY_COMMANDS}
    COMMENT "COPYING SHADERS TO OUTPUT PATH"
    VERBATIM
)
