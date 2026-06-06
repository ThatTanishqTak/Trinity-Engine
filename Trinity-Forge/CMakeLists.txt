set(TRINITY_FORGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(TRINITY_FORGE_SOURCE_DIR "${TRINITY_FORGE_ROOT}/Source")
set(TRINITY_FORGE_INCLUDE_DIR "${TRINITY_FORGE_ROOT}/Include")

trinity_add_application(Trinity-Forge "${TRINITY_FORGE_SOURCE_DIR}")

target_include_directories(Trinity-Forge
    PRIVATE
        "${TRINITY_FORGE_INCLUDE_DIR}"
        "${TRINITY_FORGE_SOURCE_DIR}"
)

target_link_libraries(Trinity-Forge
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-Forge
    PRIVATE
        TRINITY_FORGE_APPLICATION
)

trinity_set_ide_folder(Trinity-Forge "Trinity/Products")