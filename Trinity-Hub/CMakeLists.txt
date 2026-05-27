set(TRINITY_HUB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(TRINITY_HUB_SOURCE_DIR "${TRINITY_HUB_ROOT}/Source")

trinity_add_application(Trinity-Hub "${TRINITY_HUB_SOURCE_DIR}")

target_include_directories(Trinity-Hub
    PRIVATE
        "${TRINITY_HUB_SOURCE_DIR}"
)

target_link_libraries(Trinity-Hub
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-Hub
    PRIVATE
        TRINITY_HUB_APPLICATION
)

trinity_set_ide_folder(Trinity-Hub "Trinity/Products")