include_guard(GLOBAL)

function(trinity_detect_architecture OUT_VAR)
    set(_arch "${CMAKE_SYSTEM_PROCESSOR}")

    if(_arch MATCHES "AMD64|amd64|x86_64")
        set(_arch "x86_64")
    elseif(_arch MATCHES "i386|i686|x86")
        set(_arch "x86_32")
    elseif(_arch MATCHES "aarch64|arm64|ARM64")
        set(_arch "arm64")
    endif()

    if(APPLE AND CMAKE_OSX_ARCHITECTURES)
        string(REPLACE ";" "-" _arch "${CMAKE_OSX_ARCHITECTURES}")
    endif()

    set(${OUT_VAR} "${_arch}" PARENT_SCOPE)
endfunction()

function(trinity_collect_sources SOURCE_DIR OUT_VAR)
    file(GLOB_RECURSE _sources CONFIGURE_DEPENDS
        "${SOURCE_DIR}/*.h"
        "${SOURCE_DIR}/*.hpp"
        "${SOURCE_DIR}/*.inl"
        "${SOURCE_DIR}/*.c"
        "${SOURCE_DIR}/*.cc"
        "${SOURCE_DIR}/*.cpp"
        "${SOURCE_DIR}/*.cxx"
        "${SOURCE_DIR}/*.m"
        "${SOURCE_DIR}/*.mm"
    )

    set(${OUT_VAR} ${_sources} PARENT_SCOPE)
endfunction()

function(trinity_has_compilable_source SOURCES_VAR OUT_VAR)
    set(_has_source FALSE)

    foreach(_source IN LISTS ${SOURCES_VAR})
        if(_source MATCHES "\\.(c|cc|cpp|cxx|m|mm)$")
            set(_has_source TRUE)
            break()
        endif()
    endforeach()

    set(${OUT_VAR} ${_has_source} PARENT_SCOPE)
endfunction()

function(trinity_add_placeholder_source TARGET_NAME OUT_VAR)
    set(_placeholder "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_Placeholder.cpp")

    file(WRITE "${_placeholder}"
        "namespace TrinityGenerated {\n"
        "    void ${TARGET_NAME}_Placeholder() {}\n"
        "}\n"
    )

    set(${OUT_VAR} "${_placeholder}" PARENT_SCOPE)
endfunction()

function(trinity_add_placeholder_main TARGET_NAME OUT_VAR)
    set(_placeholder "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_Main.cpp")

    file(WRITE "${_placeholder}"
        "#include <iostream>\n"
        "int main(int, char**) {\n"
        "    std::cout << \"${TARGET_NAME} placeholder executable. Add real source files.\\n\";\n"
        "    return 0;\n"
        "}\n"
    )

    set(${OUT_VAR} "${_placeholder}" PARENT_SCOPE)
endfunction()

function(trinity_set_output_directories TARGET_NAME)
    if(NOT DEFINED TRINITY_OUTPUT_BASE)
        message(FATAL_ERROR "TRINITY_OUTPUT_BASE must be defined before creating targets.")
    endif()

    set_target_properties(${TARGET_NAME}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${TRINITY_OUTPUT_BASE}/${TARGET_NAME}"
            LIBRARY_OUTPUT_DIRECTORY "${TRINITY_OUTPUT_BASE}/${TARGET_NAME}"
            ARCHIVE_OUTPUT_DIRECTORY "${TRINITY_OUTPUT_BASE}/${TARGET_NAME}"
    )
endfunction()

function(trinity_apply_common_settings TARGET_NAME)
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)

    trinity_set_output_directories(${TARGET_NAME})

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            TRINITY_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
            TRINITY_VERSION_MINOR=${PROJECT_VERSION_MINOR}
            TRINITY_VERSION_PATCH=${PROJECT_VERSION_PATCH}
            $<$<CONFIG:Debug>:TRINITY_DEBUG>
            $<$<CONFIG:Release>:TRINITY_RELEASE>
            $<$<CONFIG:RelWithDebInfo>:TRINITY_RELEASE>
            $<$<CONFIG:MinSizeRel>:TRINITY_RELEASE>
    )

    if(MSVC)
        target_compile_options(${TARGET_NAME}
            PRIVATE
                /W4
                /permissive-
                /Zc:preprocessor
                /Zc:__cplusplus
        )

        if(TRINITY_WARNINGS_AS_ERRORS)
            target_compile_options(${TARGET_NAME} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${TARGET_NAME}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wshadow
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Wcast-align
                -Wunused
        )

        if(TRINITY_WARNINGS_AS_ERRORS)
            target_compile_options(${TARGET_NAME} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(trinity_set_ide_folder TARGET_NAME FOLDER_NAME)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    if(TARGET ${TARGET_NAME})
        set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "${FOLDER_NAME}")
    endif()
endfunction()

function(trinity_add_static_library TARGET_NAME SOURCE_DIR)
    trinity_collect_sources("${SOURCE_DIR}" _sources)
    trinity_has_compilable_source(_sources _has_source)

    if(NOT _has_source)
        trinity_add_placeholder_source(${TARGET_NAME} _placeholder)
        list(APPEND _sources "${_placeholder}")
    endif()

    add_library(${TARGET_NAME} STATIC ${_sources})
    trinity_apply_common_settings(${TARGET_NAME})
endfunction()

function(trinity_add_application TARGET_NAME SOURCE_DIR)
    trinity_collect_sources("${SOURCE_DIR}" _sources)
    trinity_has_compilable_source(_sources _has_source)

    if(NOT _has_source)
        trinity_add_placeholder_main(${TARGET_NAME} _placeholder)
        list(APPEND _sources "${_placeholder}")
    endif()

    add_executable(${TARGET_NAME} ${_sources})
    trinity_apply_common_settings(${TARGET_NAME})

    if(TARGET Trinity-Shaders)
        add_dependencies(${TARGET_NAME} Trinity-Shaders)

        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/Shaders"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/Shaders"
            COMMENT "Deploying shaders to ${TARGET_NAME}"
            VERBATIM
        )
    endif()

    if(TRINITY_SLANG_RUNTIME_DLLS)
        foreach(_dll ${TRINITY_SLANG_RUNTIME_DLLS})
            add_custom_command(
                TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dll}"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>"
                VERBATIM
            )
        endforeach()
    endif()
endfunction()