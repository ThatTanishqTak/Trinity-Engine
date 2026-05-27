set(TRINITY_SERVER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(TRINITY_SERVER_SOURCE_DIR "${TRINITY_SERVER_ROOT}/Source")

trinity_add_application(Trinity-Server "${TRINITY_SERVER_SOURCE_DIR}")

target_include_directories(Trinity-Server
    PRIVATE
        "${TRINITY_SERVER_SOURCE_DIR}"
)

target_link_libraries(Trinity-Server
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-Server
    PRIVATE
        TRINITY_SERVER_APPLICATION
        TRINITY_HEADLESS
)

trinity_set_ide_folder(Trinity-Server "Trinity/Products")