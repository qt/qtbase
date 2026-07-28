# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16...3.28)

project(test_cxx_modules)

# C++ module support requires CMake 3.28+ and Ninja 1.11+; skip silently otherwise.
if(CMAKE_VERSION VERSION_LESS "3.28")
    return()
endif()

execute_process(
    COMMAND "${CMAKE_MAKE_PROGRAM}" --version
    OUTPUT_VARIABLE _ninja_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(_ninja_version VERSION_LESS "1.11")
    return()
endif()

# Skip silently on toolchains too old to reliably support C++20 modules.
if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14")
        OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16"))
    return()
endif()

set(optional_modules Concurrent DBus Gui Network OpenGL OpenGLWidgets PrintSupport Sql Test Widgets Xml)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 OPTIONAL_COMPONENTS ${optional_modules})

add_executable(test_cxx_modules main.cpp)

target_sources(test_cxx_modules
    PRIVATE FILE_SET CXX_MODULES FILES mod.cppm
)

set_target_properties(test_cxx_modules PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(test_cxx_modules PRIVATE Qt::Core)

foreach(_mod IN LISTS optional_modules)
    if(TARGET Qt::${_mod})
        string(TOUPPER "${_mod}" _MOD)
        target_compile_definitions(test_cxx_modules PRIVATE HAS_QT${_MOD})
        target_link_libraries(test_cxx_modules PRIVATE Qt::${_mod})
    endif()
endforeach()
