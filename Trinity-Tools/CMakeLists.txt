set(TRINITY_TOOLS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

trinity_add_application(
    Trinity-BuildTool
    "${TRINITY_TOOLS_ROOT}/Trinity-BuildTool/Source"
)

target_link_libraries(Trinity-BuildTool
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-BuildTool
    PRIVATE
        TRINITY_BUILD_TOOL
)

trinity_set_ide_folder(Trinity-BuildTool "Trinity/Tools")

trinity_add_application(
    Trinity-Cooker
    "${TRINITY_TOOLS_ROOT}/Trinity-Cooker/Source"
)

target_link_libraries(Trinity-Cooker
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-Cooker
    PRIVATE
        TRINITY_COOKER_TOOL
)

trinity_set_ide_folder(Trinity-Cooker "Trinity/Tools")

trinity_add_application(
    Trinity-ShaderCompiler
    "${TRINITY_TOOLS_ROOT}/Trinity-ShaderCompiler/Source"
)

target_link_libraries(Trinity-ShaderCompiler
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-ShaderCompiler
    PRIVATE
        TRINITY_SHADER_COMPILER_TOOL
)

trinity_set_ide_folder(Trinity-ShaderCompiler "Trinity/Tools")

trinity_add_application(
    Trinity-PackageTool
    "${TRINITY_TOOLS_ROOT}/Trinity-PackageTool/Source"
)

target_link_libraries(Trinity-PackageTool
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-PackageTool
    PRIVATE
        TRINITY_PACKAGE_TOOL
)

trinity_set_ide_folder(Trinity-PackageTool "Trinity/Tools")

trinity_add_application(
    Trinity-ProjectGenerator
    "${TRINITY_TOOLS_ROOT}/Trinity-ProjectGenerator/Source"
)

target_link_libraries(Trinity-ProjectGenerator
    PRIVATE
        Trinity::Engine
)

target_compile_definitions(Trinity-ProjectGenerator
    PRIVATE
        TRINITY_PROJECT_GENERATOR_TOOL
)

trinity_set_ide_folder(Trinity-ProjectGenerator "Trinity/Tools")