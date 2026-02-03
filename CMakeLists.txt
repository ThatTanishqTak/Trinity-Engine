cmake_minimum_required(VERSION 3.30)
project(Trinity)

if (MSVC)
    add_compile_options(/MP)
endif()

if (_WIN32)
    target_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})
if(ARCH_NAME MATCHES AMD64|x86_64)
    set(ARCH_NAME x86_64)
elseif(ARCH_NAME MATCHES i386|i686)
    set(ARCH_NAME x86_32)
endif()

set(OUTPUT_BASE ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${CMAKE_SYSTEM_NAME}-${ARCH_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BASE}/${PROJECT_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BASE}/${PROJECT_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BASE}/${PROJECT_NAME})

add_subdirectory(Trinity-Engine)
add_subdirectory(Trinity-Forge)