set(TRINITY_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(TRINITY_RUNTIME_SOURCE_DIR "${TRINITY_RUNTIME_ROOT}/Source")

trinity_add_application(Trinity-Runtime "${TRINITY_RUNTIME_SOURCE_DIR}")

target_include_directories(Trinity-Runtime
    PRIVATE
        "${TRINITY_RUNTIME_SOURCE_DIR}"
)

target_link_libraries(Trinity-Runtime
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-Runtime
    PRIVATE
        TRINITY_RUNTIME_APPLICATION
)

trinity_set_ide_folder(Trinity-Runtime "Trinity/Products")