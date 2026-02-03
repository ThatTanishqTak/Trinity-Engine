project(TrinityForge)

file(GLOB_RECURSE FORGE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
)

add_executable(${PROJECT_NAME} ${FORGE_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    TrinityEngine
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Trinity-Engine/src
)